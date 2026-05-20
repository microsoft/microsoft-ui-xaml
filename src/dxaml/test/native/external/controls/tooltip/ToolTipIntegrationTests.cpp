// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ToolTipIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <ControlHelper.h>
#include <FlyoutHelper.h>
#include <FocusTestHelper.h>
#include "FocusTestHelper.h"
#include "WUCRenderingScopeGuard.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ToolTip {

    bool ToolTipIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ToolTipIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ToolTipIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ToolTipIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ToolTip>::CanInstantiate();
    }

    void ToolTipIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ToolTip>::CanEnterAndLeaveLiveTree();
    }

    void ToolTipIntegrationTests::CanSetToolTipBeforeAndAfterManagedPeerCreation()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ grid = nullptr;

        RunOnUIThread([&]()
        {
            grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' ToolTipService.ToolTip='Tool tip!' />"));

            TestServices::WindowHelper->WindowContent = grid;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            Platform::String^ s = L"New tool tip!";
            xaml_controls::ToolTipService::SetToolTip(grid, s);
            Platform::String^ setString = safe_cast<Platform::String^>(xaml_controls::ToolTipService::GetToolTip(grid));

            VERIFY_IS_NOT_NULL(setString);
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(s, setString) == 0);
        });
    }

    void ToolTipIntegrationTests::CanSetAndGetOffsetProperties()
    {
        TestCleanupWrapper cleanup;

        const double horizontalOffset = 20;
        const double verticalOffset = 10;

        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();
        xaml_controls::Border^ border = nullptr;

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
            // Verify default values for Tooltip Offset properties.
            VERIFY_ARE_EQUAL(0, toolTip->HorizontalOffset);
            VERIFY_ARE_EQUAL(0, toolTip->VerticalOffset);

            // Set the Tooltip Offset properties.
            toolTip->HorizontalOffset = horizontalOffset;
            toolTip->VerticalOffset = verticalOffset;

            xaml_controls::ToolTipService::SetPlacement(button, xaml_primitives::PlacementMode::Top);
            toolTip->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // For Top PlacementMode, verify the offset properties.
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(horizontalOffset, toolTip->HorizontalOffset);
            VERIFY_ARE_EQUAL(verticalOffset, toolTip->VerticalOffset);

            auto offset = button->TransformToVisual(toolTip)->TransformPoint(wf::Point(0, 0));
            VERIFY_ARE_EQUAL(offset.X, (toolTip->ActualWidth - button->ActualWidth) / 2);
            VERIFY_ARE_EQUAL(offset.Y, toolTip->ActualHeight + verticalOffset);

            toolTip->IsOpen = false;
        });

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetPlacement(button, xaml_primitives::PlacementMode::Right);
            toolTip->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // For Right PlacementMode, verify the offset properties.
        RunOnUIThread([&]()
        {
            auto offset = toolTip->TransformToVisual(button)->TransformPoint(wf::Point(0, 0));
            VERIFY_ARE_EQUAL(offset.X, button->ActualWidth + horizontalOffset);
            VERIFY_ARE_EQUAL(offset.Y, (button->ActualHeight - toolTip->ActualHeight) / 2);

            toolTip->IsOpen = false;
        });

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetPlacement(button, xaml_primitives::PlacementMode::Bottom);
            toolTip->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // For Bottom PlacementMode, verify the offset properties.
        RunOnUIThread([&]()
        {
            auto offset = toolTip->TransformToVisual(button)->TransformPoint(wf::Point(0, 0));
            VERIFY_ARE_EQUAL(offset.X, (button->ActualWidth - toolTip->ActualWidth) / 2);
            VERIFY_ARE_EQUAL(offset.Y, button->ActualHeight + verticalOffset);

            toolTip->IsOpen = false;
        });

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetPlacement(button, xaml_primitives::PlacementMode::Left);
            toolTip->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // For Left PlacementMode, verify the offset properties.
        RunOnUIThread([&]()
        {
            auto offset = button->TransformToVisual(toolTip)->TransformPoint(wf::Point(0, 0));
            VERIFY_ARE_EQUAL(offset.X, toolTip->ActualWidth + horizontalOffset);
            VERIFY_ARE_EQUAL(offset.Y, (toolTip->ActualHeight - button->ActualHeight) / 2);

            toolTip->IsOpen = false;
        });
    }

    //
    // Verify the ToolTip open and close.
    //
    void ToolTipIntegrationTests::CanToolTipOpenCloseProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanToolTipOpenClose();
    }

    void ToolTipIntegrationTests::CanToolTipOpenCloseDropShadow()
    {
        CanToolTipOpenClose();
    }

    void ToolTipIntegrationTests::CanToolTipOpenClose()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_controls::Button^ button = nullptr;

        auto toolTip = CreateToolTip();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='Bisque' Width='400' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Border  Background='SlateBlue' Width='400' Height='250'> "
                L"    <Button x:Name='button' Content='button.tooltip' VerticalAlignment='Center' HorizontalAlignment='Center' FontSize='25' > "
                L"    </Button> "
                L"  </Border> "
                L"</Grid>"));

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Add the dummy input to ensure the clicking button.
        TestServices::InputHelper->Tap(wf::Point(5, 5));

        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Top, xaml_primitives::PlacementMode::Top, InputMode::Touch, true);
    }

    void ToolTipIntegrationTests::ValidateCanPlaceToolTipOnHyperlink()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Button^ buttonForFocus = nullptr;
        xaml_controls::RichTextBlock^ richTextBlock1 = nullptr;
        xaml_controls::RichTextBlock^ richTextBlock2 = nullptr;
        xaml_controls::ToolTip^ toolTip1 = nullptr;
        xaml_controls::ToolTip^ toolTip2 = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto toolTip1OpenedEvent = std::make_shared<Event>();
        auto toolTip1ClosedEvent = std::make_shared<Event>();
        auto toolTip2OpenedEvent = std::make_shared<Event>();
        auto toolTip2ClosedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto toolTip1OpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTip1ClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto toolTip2OpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTip2ClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='ButtonForFocus' Content='Button for focus' />
                    <RichTextBlock x:Name='RichTextBlock1'>
                        <RichTextBlock.Blocks>
                            <Paragraph>
                                <Hyperlink NavigateUri='http://www.example.com'>
                                    <ToolTipService.ToolTip>
                                        <ToolTip x:Name='ToolTip1'>http://www.example.com</ToolTip>
                                    </ToolTipService.ToolTip>
                                    Click me
                                </Hyperlink>
                            </Paragraph>
                        </RichTextBlock.Blocks>
                    </RichTextBlock>
                    <RichTextBlock x:Name='RichTextBlock2'>
                        <RichTextBlock.Blocks>
                            <Paragraph>
                                <InlineUIContainer>
                                    <HyperlinkButton NavigateUri='http://www.example.com'>
                                        <ToolTipService.ToolTip>
                                            <ToolTip x:Name='ToolTip2'>http://www.example.com</ToolTip>
                                        </ToolTipService.ToolTip>
                                        Click me
                                    </HyperlinkButton>
                                </InlineUIContainer>
                            </Paragraph>
                        </RichTextBlock.Blocks>
                    </RichTextBlock>
                </StackPanel>)"));

            buttonForFocus = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"ButtonForFocus"));
            richTextBlock1 = safe_cast<xaml_controls::RichTextBlock^>(rootPanel->FindName(L"RichTextBlock1"));
            richTextBlock2 = safe_cast<xaml_controls::RichTextBlock^>(rootPanel->FindName(L"RichTextBlock2"));
            toolTip1 = safe_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"ToolTip1"));
            toolTip2 = safe_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"ToolTip2"));

            toolTip1OpenedRegistration.Attach(toolTip1, [toolTip1OpenedEvent]() { toolTip1OpenedEvent->Set(); });
            toolTip1ClosedRegistration.Attach(toolTip1, [toolTip1ClosedEvent]() { toolTip1ClosedEvent->Set(); });
            toolTip2OpenedRegistration.Attach(toolTip2, [toolTip2OpenedEvent]() { toolTip2OpenedEvent->Set(); });
            toolTip2ClosedRegistration.Attach(toolTip2, [toolTip2ClosedEvent]() { toolTip2ClosedEvent->Set(); });

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = rootPanel;

            rootPanel->Focus(FocusState::Keyboard);
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->MoveMouse(wf::Point(0,0));
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->MoveMouse(richTextBlock1);
        toolTip1OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot)->Size);
        });

        // Ctrl to cancel ToolTip
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$u$_ctrlscan");
        toolTip1ClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot)->Size);
        });

        TestServices::InputHelper->MoveMouse(richTextBlock2);
        toolTip2OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot)->Size);
        });

        // Ctrl to cancel ToolTip
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$u$_ctrlscan");
        toolTip2ClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot)->Size);
        });

        FocusTestHelper::EnsureFocus(buttonForFocus, FocusState::Keyboard);

        TestServices::KeyboardHelper->Tab();
        toolTip1OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot)->Size);
        });

        TestServices::KeyboardHelper->Tab();
        toolTip1ClosedEvent->WaitForDefault();
        toolTip2OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot)->Size);
        });

        TestServices::KeyboardHelper->Tab();
        toolTip2ClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::VerifyToolTipPlacement()
    {
        TestCleanupWrapper cleanup;

        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
        });

        LOG_OUTPUT(L"VerifyToolTipPlacement: Verify the placement on the top of button using touch.");
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Top, xaml_primitives::PlacementMode::Top, InputMode::Touch);

        LOG_OUTPUT(L"VerifyToolTipPlacement: Verify the placement on the right of button using touch.");
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Right, xaml_primitives::PlacementMode::Right, InputMode::Touch);

        LOG_OUTPUT(L"VerifyToolTipPlacement: Verify the placement on the bottom of button using touch.");
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Bottom, xaml_primitives::PlacementMode::Bottom, InputMode::Touch);

        LOG_OUTPUT(L"VerifyToolTipPlacement: Verify the placement on the left of button using touch.");
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Left, xaml_primitives::PlacementMode::Left, InputMode::Touch);
    }

    void ToolTipIntegrationTests::PerformToolTipPlacement(
        xaml_controls::Button^ button,
        xaml_controls::ToolTip^ toolTip,
        xaml_primitives::PlacementMode mode,
        xaml_primitives::PlacementMode modeExpected,
        InputMode inputMode,
        bool validateDCompTree)
    {
        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetPlacement(button, mode);
        });
        OpenToolTip(button, toolTip, inputMode, false /* isTargetPosition */, wf::Point(0, 0));

        if (validateDCompTree)
        {
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml_controls::ToolTipService::GetPlacement(button), mode);
            VerifyToolTipPosition(button, toolTip, modeExpected);
        });
        CloseToolTip(toolTip, inputMode);
    }

    void ToolTipIntegrationTests::VerifyPlacementModeMouse()
    {
        TestCleanupWrapper cleanup;
        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();

        LOG_OUTPUT(L"VerifyPlacementModeMouse: Verify the placement mode mouse.");
        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
            xaml_controls::ToolTipService::SetPlacement(button, xaml_primitives::PlacementMode::Mouse);
        });

        OpenToolTip(button, toolTip, InputMode::Mouse, false /*isTargetPosition*/, wf::Point(0, 0));

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
            VERIFY_ARE_EQUAL(xaml_controls::ToolTipService::GetPlacement(button), xaml_primitives::PlacementMode::Mouse);
        });

        CloseToolTip(toolTip, InputMode::Mouse);
    }

    xaml_controls::Button^ ToolTipIntegrationTests::SetupToolTipTest()
    {
        xaml_controls::Button^ button = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            auto border = ref new xaml_controls::Border();

            button = ref new xaml_controls::Button();
            button->Content = L"Button.ToolTip";
            button->HorizontalAlignment = xaml::HorizontalAlignment::Center;

            border->Width = 400;
            border->Height = 250;
            border->Child = button;

            rootPanel->Children->Append(border);

            loadedRegistration.Attach(
                rootPanel,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"SetupToolTipTest: Loaded event fired!");
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        return button;
    }

    xaml_controls::Button^ ToolTipIntegrationTests::SetupOutOfWindowToolTipTest()
    {
        xaml_controls::Button^ button = nullptr;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();

            button = ref new xaml_controls::Button();
            button->Content = L"Button.OutOfWindow.ToolTip";
            button->HorizontalAlignment = xaml::HorizontalAlignment::Stretch;
            button->VerticalAlignment = xaml::VerticalAlignment::Stretch;
            button->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::BlueViolet);

            rootPanel->Children->Append(button);
            TestServices::WindowHelper->WindowContent = rootPanel;

            rootPanel->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();
        return button;
    }

    xaml_controls::Button^ ToolTipIntegrationTests::SetupScaledToolTipTest()
    {
        xaml_controls::Button^ button = nullptr;

        TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(400, 600), 2.f);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            auto border = ref new xaml_controls::Border();

            button = ref new xaml_controls::Button();
            button->Content = L"Scaled.ToolTip";
            button->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            button->VerticalAlignment = xaml::VerticalAlignment::Top;
            button->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Orange);
            button->Margin = xaml::Thickness({ 20, 100, 20, 0 });
            button->Width = 100;
            button->Height = 50;

            border->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::SlateBlue);
            border->Child = button;

            rootPanel->Children->Append(border);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        return button;
    }

    xaml_controls::ToolTip^ ToolTipIntegrationTests::CreateToolTip()
    {
        xaml_controls::ToolTip^ toolTip = nullptr;

        RunOnUIThread([&]()
        {
            toolTip = ref new xaml_controls::ToolTip();
            VERIFY_IS_NOT_NULL(toolTip);

            auto textBlock = ref new xaml_controls::TextBlock();
            VERIFY_IS_NOT_NULL(textBlock);

            textBlock->Text = L"look...  its a tooltip";
            toolTip->Content = textBlock;
        });

        return toolTip;
    }

    xaml_controls::ToolTip^ ToolTipIntegrationTests::CreateLongToolTip()
    {
        xaml_controls::ToolTip^ toolTip = nullptr;

        RunOnUIThread([&]()
        {
            toolTip = ref new xaml_controls::ToolTip();
            VERIFY_IS_NOT_NULL(toolTip);

            auto textBlock = ref new xaml_controls::TextBlock();
            VERIFY_IS_NOT_NULL(textBlock);

            textBlock->Text = L"look...  its a tooltip! look...  its a tooltip!";
            toolTip->Content = textBlock;
        });

        return toolTip;
    }

    xaml_controls::ToolTip^ ToolTipIntegrationTests::CreateTallToolTip()
    {
        xaml_controls::ToolTip^ toolTip = nullptr;

        RunOnUIThread([&]()
        {
            toolTip = ref new xaml_controls::ToolTip();

            auto textBlock = ref new xaml_controls::TextBlock();

            Platform::String^ textBlockText = L"look...  its a tooltip!";

            for (int i = 0; i < 200; i++)
            {
                textBlockText += L"\r\nlook...  its a tooltip!";
            }

            textBlock->Text = textBlockText;
            toolTip->Content = textBlock;
        });

        return toolTip;
    }

    void ToolTipIntegrationTests::OpenToolTip(xaml_controls::Button^ button, xaml_controls::ToolTip^ toolTip, InputMode inputMode, bool isTargetPosition, wf::Point point)
    {
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        clickRegistration.Attach(button, [&]()
        {
            LOG_OUTPUT(L"OpenToolTip: Click event fired on the button!");
            toolTip->IsOpen = true;
        });

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        openedRegistration.Attach(toolTip, [&]()
        {
            LOG_OUTPUT(L"OpenToolTip: ToolTip Opened event fired!");
            openedEvent->Set();
        });

        TestServices::WindowHelper->WaitForIdle();

        if (inputMode == InputMode::Mouse)
        {
            if (isTargetPosition)
            {
                TestServices::InputHelper->MoveMouse(point);
            }
            else
            {
                // In case the mouse was already over the target, we first move the mouse away before moving it back.
                TestServices::InputHelper->MoveMouse(wf::Point(0,0));
                TestServices::WindowHelper->WaitForIdle();
                TestServices::InputHelper->MoveMouse(button);
            }
        }
        else if (inputMode == InputMode::Touch)
        {
            TestServices::InputHelper->Tap(button);
        }
        else if (inputMode == InputMode::UIA)
        {
            RunOnUIThread([&]()
            {
                auto buttonAp = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button));
                buttonAp->SetFocus();
            });
        }
        else
        {
            RunOnUIThread([&]()
            {
                toolTip->IsOpen = true;
            });
        }

        openedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::CloseToolTip(xaml_controls::ToolTip^ toolTip, InputMode inputMode)
    {
        auto closedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            closedRegistration.Attach(
                toolTip,
                ref new xaml::RoutedEventHandler([closedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"CloseToolTip: ToolTip Closed event fired!");
                closedEvent->Set();
            }));

            if (inputMode != InputMode::Mouse)
            {
                LOG_OUTPUT(L"CloseToolTip: Close tooltip by IsOpen=FALSE.");
                toolTip->IsOpen = false;
            }
        });

        if (inputMode == InputMode::Mouse)
        {
            LOG_OUTPUT(L"CloseToolTip: Close tooltip by moving mouse.");
            TestServices::InputHelper->MoveMouse(wf::Point(0, 0));
            TestServices::WindowHelper->WaitForIdle();
        }
        else if (inputMode == InputMode::Keyboard)
        {
            LOG_OUTPUT(L"Pressing Ctrl to close tooltip.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$u$_ctrlscan");
        }

        closedEvent->WaitForDefault();
    }

    void ToolTipIntegrationTests::VerifyToolTipPosition(
        xaml::FrameworkElement^ target,
        xaml::FrameworkElement^ toolTip,
        xaml_primitives::PlacementMode modeExpected)
    {
        wf::Rect ttBounds = ControlHelper::GetBounds(toolTip);
        wf::Rect targetBounds = ControlHelper::GetBounds(target);

        LOG_OUTPUT(L"ToolTip bounds left=%f top=%f width=%f height=%f", ttBounds.Left, ttBounds.Top, ttBounds.Width, ttBounds.Height);
        LOG_OUTPUT(L"Target bounds left=%f top=%f width=%f height=%f", targetBounds.Left, targetBounds.Top, targetBounds.Width, targetBounds.Height);

        switch (modeExpected)
        {
        case xaml_primitives::PlacementMode::Left:
            VERIFY_IS_TRUE(ttBounds.X + ttBounds.Width <= targetBounds.X + 1);
            break;
        case xaml_primitives::PlacementMode::Right:
            VERIFY_IS_TRUE(ttBounds.X > targetBounds.X);
            VERIFY_IS_TRUE(ttBounds.X + ttBounds.Width > targetBounds.X + targetBounds.Width);
            VERIFY_IS_TRUE(ttBounds.X + 1 >= targetBounds.X + targetBounds.Width);
            break;
        case xaml_primitives::PlacementMode::Top:
            VERIFY_IS_TRUE(ttBounds.Y + ttBounds.Height <= targetBounds.Y + 1);
            break;
        case xaml_primitives::PlacementMode::Bottom:
            VERIFY_IS_TRUE(ttBounds.Y + 1 >= targetBounds.Y + targetBounds.Height);
            break;
        }
    }

    void ToolTipIntegrationTests::ValidateUIETreeForMouse()
    {
        ValidateUIETree(InputMode::Mouse);
    }

    void ToolTipIntegrationTests::ValidateUIETreeForKeyboard()
    {
        ValidateUIETree(InputMode::Keyboard);
    }

    void ToolTipIntegrationTests::ValidateUIETreeForTouch()
    {
        ValidateUIETree(InputMode::Touch);
    }

    void ToolTipIntegrationTests::ValidateUIETree(InputMode mode)
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 700),
            1.f,
            // Test setup.
            [mode]()
            {
                xaml_controls::Panel^ rootPanel = nullptr;

                auto button = SetupToolTipTest();
                auto toolTip = CreateToolTip();

                RunOnUIThread([&]()
                {
                    xaml_controls::ToolTipService::SetToolTip(button, toolTip);

                    rootPanel = safe_cast<xaml_controls::Panel^>(TestServices::WindowHelper->WindowContent);
                });
                TestServices::WindowHelper->WaitForIdle();

                OpenToolTip(button, toolTip, mode, false /* isTargetPosition */, wf::Point(0, 0));

                return rootPanel;
            },
            // Test cleanup.
            [mode]()
            {
                xaml_controls::ToolTip^ toolTip = nullptr;

                RunOnUIThread([&]()
                {
                    auto root = safe_cast<xaml_controls::Panel^>(TestServices::WindowHelper->WindowContent);
                    auto border = safe_cast<xaml_controls::Border^>(root->Children->GetAt(0));
                    auto button = safe_cast<xaml_controls::Button^>(border->Child);
                    toolTip = safe_cast<xaml_controls::ToolTip^>(xaml_controls::ToolTipService::GetToolTip(button));
                });

                CloseToolTip(toolTip, InputMode::None);
            },
            false /*disableHittestingOnRoot*/);
    }

    void ToolTipIntegrationTests::ValidateOutOfWindowToolTipPlacement()
    {
        TestCleanupWrapper cleanup;

        auto button = SetupOutOfWindowToolTipTest();
        auto toolTip = CreateLongToolTip();

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
        });

        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Top, xaml_primitives::PlacementMode::Top, InputMode::Touch);
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Right, xaml_primitives::PlacementMode::Right, InputMode::Touch);
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Bottom, xaml_primitives::PlacementMode::Bottom, InputMode::Touch);
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Left, xaml_primitives::PlacementMode::Right, InputMode::Touch);
    }

    void ToolTipIntegrationTests::ValidateOutOfWindowRTLToolTipPlacementWithMouse()
    {
        TestCleanupWrapper cleanup;

        auto button = SetupOutOfWindowToolTipTest();
        auto toolTip = CreateLongToolTip();

        RunOnUIThread([&]()
        {
            button->FlowDirection = xaml::FlowDirection::RightToLeft;
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
            xaml_controls::ToolTipService::SetPlacement(button, xaml_primitives::PlacementMode::Mouse);
        });

        // Open the RTL ToolTip at Point(50, 50). Since 50px is not enough for this long ToolTip,
        // its left edge will be up against the left edge of the XAML window.
        OpenToolTip(button, toolTip, InputMode::Mouse, true /*isTargetPosition*/, wf::Point(50, 50));

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);

            auto actualBounds = ControlHelper::GetBounds(toolTip);
            wf::Rect expectedBounds(1.0, 60, 223, 30);
            LOG_OUTPUT(L"ValidateOutOfWindowRTLToolTipPlacementWithMouse: ToolTip bounds left=%f top=%f width=%f height=%f", actualBounds.Left, actualBounds.Top, actualBounds.Width, actualBounds.Height);
            LOG_OUTPUT(L"ValidateOutOfWindowRTLToolTipPlacementWithMouse: Expected bounds left=%f top=%f width=%f height=%f", expectedBounds.Left, expectedBounds.Top, expectedBounds.Width, expectedBounds.Height);

            VERIFY_IS_TRUE(ControlHelper::AreClose(expectedBounds.X, actualBounds.X, 2.0));
            VERIFY_IS_TRUE(ControlHelper::AreClose(expectedBounds.Y, actualBounds.Y, 2.0));
            VERIFY_IS_TRUE(ControlHelper::AreClose(expectedBounds.Width, actualBounds.Width, 2.0));
            VERIFY_IS_TRUE(ControlHelper::AreClose(expectedBounds.Height, actualBounds.Height, 2.0));
        });

        // because of ToolTip safezone, mouse can't close the tooltip
        CloseToolTip(toolTip, InputMode::Keyboard);
    }

    void ToolTipIntegrationTests::ValidateOutOfWindowToolTipPlacementWithMouse()
    {
        TestCleanupWrapper cleanup;

        auto button = SetupOutOfWindowToolTipTest();
        auto toolTip = CreateLongToolTip();
        wf::Rect ttBounds;

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
            xaml_controls::ToolTipService::SetPlacement(button, xaml_primitives::PlacementMode::Mouse);
        });

        // Open the ToolTip at Point(300, 100) that will be located to the out of Xaml window
        OpenToolTip(button, toolTip, InputMode::Mouse, true /*isTargetPosition*/, wf::Point(300, 100));

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);

            ttBounds = ControlHelper::GetBounds(toolTip);
            LOG_OUTPUT(L"ValidateOutOfWindowToolTipPlacementWithMouse: ToolTip bounds left=%f top=%f width=%f height=%f", ttBounds.Left, ttBounds.Top, ttBounds.Width, ttBounds.Height);

            VERIFY_ARE_EQUAL(299, ttBounds.X);
            VERIFY_ARE_EQUAL(110, ttBounds.Y);
            VERIFY_ARE_EQUAL(223, ttBounds.Width);
            VERIFY_ARE_EQUAL(30, ttBounds.Height);
        });

        // because of ToolTip safezone, mouse can't close the tooltip
        CloseToolTip(toolTip, InputMode::Keyboard);
    }

    void ToolTipIntegrationTests::ValidateScaledToolTipPlacement()
    {
        TestCleanupWrapper cleanup;

        auto button = SetupScaledToolTipTest();
        auto toolTip = CreateLongToolTip();

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
        });

        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Top, xaml_primitives::PlacementMode::Top, InputMode::None);
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Right, xaml_primitives::PlacementMode::Right, InputMode::None);
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Bottom, xaml_primitives::PlacementMode::Bottom, InputMode::None);
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Left, xaml_primitives::PlacementMode::Right, InputMode::None);
    }

    void ToolTipIntegrationTests::ValidateRTLToolTipPlacement()
    {
        TestCleanupWrapper cleanup;

        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();

        RunOnUIThread([&]()
        {
            button->FlowDirection = xaml::FlowDirection::RightToLeft;
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
        });

        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Left, xaml_primitives::PlacementMode::Right, InputMode::None);
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Right, xaml_primitives::PlacementMode::Left, InputMode::None);
    }

    void ToolTipIntegrationTests::ValidateNestedToolTips()
    {
        TestCleanupWrapper cleanup;

        // This test covers the case of nested ToolTips: i.e. the case of having one element as a child of another, both having ToolTips.
        // When the mouse is over the parent element, but not the child element, the parent's tooltip should show.
        // When the mouse is over the child element (and hence also over the parent element), the child's tooltip should show.
        // When the mouse leaves both elements, no tooltips should show.

        // This test adds regression coverage for:
        // Tooltips showing incorrect content at incorrect times
        // In this bug, the parent ToolTip was still showing up even after we moved the mouse away from the element.

        xaml_controls::StackPanel^ rootPanel;
        xaml_controls::ToolTip^ parentToolTip;
        xaml_controls::ToolTip^ nestedToolTip;
        xaml::FrameworkElement^ parentTarget;
        xaml::FrameworkElement^ nestedTarget;
        xaml::FrameworkElement^ noToolTipElement;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L""
                L"    <StackPanel x:Name='parentTarget' Width='200' Background='Yellow' >"
                L"        <ToolTipService.ToolTip>"
                L"            <ToolTip x:Name='parentToolTip'>"
                L"                Parent ToolTip"
                L"            </ToolTip>"
                L"        </ToolTipService.ToolTip>"
                L"        <TextBlock Text='Parent Target' Height='100'  />"
                L"        <Border x:Name='nestedTarget' Height='50' BorderBrush='Red' BorderThickness='2' Background='Green' >"
                L"            <ToolTipService.ToolTip>"
                L"                <ToolTip x:Name='nestedToolTip'>"
                L"                    Nested ToolTip"
                L"                </ToolTip>"
                L"            </ToolTipService.ToolTip>"
                L"            <TextBlock Text='Nested Target'  />"
                L"        </Border>"
                L"    </StackPanel>"
                L""
                L"    <Border x:Name='noToolTipElement' Height='100' BorderBrush='Red' BorderThickness='2' Background='LightBlue' CornerRadius='5' Margin='10' >"
                L"        <TextBlock Text='Element with no ToolTip'  />"
                L"    </Border>"
                L"</StackPanel>"));
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Clicking on root to force mouse events");
        test_infra::TestServices::InputHelper->LeftMouseClick(rootPanel);
        TestServices::WindowHelper->WaitForIdle();

        auto parentToolTipOpenedEvent = std::make_shared<Event>();
        auto parentToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto parentToolTipClosedEvent = std::make_shared<Event>();
        auto parentToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto nestedToolTipOpenedEvent = std::make_shared<Event>();
        auto nestedToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto nestedToolTipClosedEvent = std::make_shared<Event>();
        auto nestedToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            parentToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"parentToolTip"));
            nestedToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"nestedToolTip"));
            parentTarget = dynamic_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"parentTarget"));
            nestedTarget = dynamic_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"nestedTarget"));
            noToolTipElement = dynamic_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"noToolTipElement"));

            parentToolTipOpenedRegistration.Attach(parentToolTip, ref new xaml::RoutedEventHandler([parentToolTipOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: Parent ToolTip Opened event fired!");
                parentToolTipOpenedEvent->Set();
            }));
            parentToolTipClosedRegistration.Attach(parentToolTip, ref new xaml::RoutedEventHandler([parentToolTipClosedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: Parent ToolTip Closed event fired!");
                parentToolTipClosedEvent->Set();
            }));

            nestedToolTipOpenedRegistration.Attach(nestedToolTip, ref new xaml::RoutedEventHandler([nestedToolTipOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: Nested ToolTip Opened event fired!");
                nestedToolTipOpenedEvent->Set();
            }));
            nestedToolTipClosedRegistration.Attach(nestedToolTip, ref new xaml::RoutedEventHandler([nestedToolTipClosedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: Nested ToolTip Closed event fired!");
                nestedToolTipClosedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse to Parent element");
        TestServices::InputHelper->MoveMouse(parentTarget);
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for Parent ToolTip to open");
        parentToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"We expect the Parent ToolTip to open. Nothing else should open or close. ");
        VERIFY_IS_TRUE(parentToolTipOpenedEvent->HasFired());
        VERIFY_IS_FALSE(parentToolTipClosedEvent->HasFired());
        VERIFY_IS_FALSE(nestedToolTipOpenedEvent->HasFired());
        VERIFY_IS_FALSE(nestedToolTipClosedEvent->HasFired());

        parentToolTipOpenedEvent->Reset();

        LOG_OUTPUT(L"Moving mouse to nested element");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MoveMouse(nestedTarget);
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for parent ToolTip to close and nested ToolTip to open");
        parentToolTipClosedEvent->WaitForDefault();
        nestedToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"We expect the Parent ToolTip to close and the nested ToolTip to open. Nothing else should open or close.");
        VERIFY_IS_FALSE(parentToolTipOpenedEvent->HasFired());
        VERIFY_IS_TRUE(parentToolTipClosedEvent->HasFired());
        VERIFY_IS_TRUE(nestedToolTipOpenedEvent->HasFired());
        VERIFY_IS_FALSE(nestedToolTipClosedEvent->HasFired());

        parentToolTipClosedEvent->Reset();
        nestedToolTipOpenedEvent->Reset();

        LOG_OUTPUT(L"Moving mouse to element with no ToolTip");
        TestServices::InputHelper->MoveMouse(noToolTipElement);
        LOG_OUTPUT(L"Waiting for nested ToolTip to close");
        nestedToolTipClosedEvent->WaitForDefault();

        // TODO: ToolTipIntegrationTests::ValidateNestedToolTips should wait for DispatcherTimers to complete
        // We need WaitForIdle to wait for all DispatcherTimers to complete, but that is not currently supported.
        // We're trying to prove a negative here. We want to verify that NO ToolTip opens after we move the mouse away.
        // We want to verify that parentToolTipOpenedEvent does NOT fire. So there is no event for us to wait on.
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"We expect the nested ToolTip to close, and nothing else to open or close.");
        VERIFY_IS_FALSE(parentToolTipOpenedEvent->HasFired());
        VERIFY_IS_FALSE(parentToolTipClosedEvent->HasFired());
        VERIFY_IS_FALSE(nestedToolTipOpenedEvent->HasFired());
        VERIFY_IS_TRUE(nestedToolTipClosedEvent->HasFired());
    }

    void ToolTipIntegrationTests::ValidateToolTipPositionOnSliderWithMouse()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Slider^ slider = nullptr;
        xaml_primitives::Thumb^ thumb = nullptr;
        wf::Rect sliderBounds = {};
        wf::Rect thumbBounds = {};

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='Bisque' > "
                L"    <Slider x:Name='sliderControl' HorizontalAlignment='Center' VerticalAlignment='Center' Minimum='0' Maximum='100' Width='300' IsThumbToolTipEnabled='True' /> "
                L"</Grid>"));

            slider = dynamic_cast<xaml_controls::Slider^>(rootPanel->FindName(L"sliderControl"));
            VERIFY_IS_NOT_NULL(slider);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            thumb = safe_cast<xaml_primitives::Thumb^>(TreeHelper::GetVisualChildByName(slider, L"HorizontalThumb"));
            VERIFY_IS_NOT_NULL(thumb);

            sliderBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(slider));
            LOG_OUTPUT(L"Slider bounds left=%f top=%f width=%f height=%f", sliderBounds.Left, sliderBounds.Top, sliderBounds.Width, sliderBounds.Height);
            thumbBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(thumb));
            LOG_OUTPUT(L"Thumb bounds left=%f top=%f width=%f height=%f", thumbBounds.Left, thumbBounds.Top, thumbBounds.Width, thumbBounds.Height);
        });

        // Drag the slider's thumb to the middle of slider
        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        TestServices::InputHelper->MouseDrag(
            wf::Point(thumbBounds.Left + (thumbBounds.Width / 2), thumbBounds.Top + (thumbBounds.Height / 2)),
            wf::Point(thumbBounds.Left + (thumbBounds.Width / 2) + 150, thumbBounds.Top + (thumbBounds.Height / 2)),
            MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);

            auto popup = popups->GetAt(0);
            auto toolTip = popup->Child;

            wf::Rect toolTipBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(toolTip));
            LOG_OUTPUT(L"ToolTip bounds left=%f top=%f width=%f height=%f at the middle of slider", toolTipBounds.Left, toolTipBounds.Top, toolTipBounds.Width, toolTipBounds.Height);

            VERIFY_IS_TRUE(toolTipBounds.Left > sliderBounds.Left && toolTipBounds.Left + toolTipBounds.Width < sliderBounds.Left + sliderBounds.Width);
            VERIFY_IS_TRUE(toolTipBounds.Top < sliderBounds.Top);
        });
        TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        // Drag the slider's thumb to the end of slider
        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        TestServices::InputHelper->MouseDrag(
            wf::Point(thumbBounds.Left + (thumbBounds.Width / 2), thumbBounds.Top + (thumbBounds.Height / 2)),
            wf::Point(thumbBounds.Left + (thumbBounds.Width / 2) + 300, 0),
            MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot) ;
            auto popup = popups->GetAt(0);
            auto toolTip = popup->Child;

            wf::Rect toolTipBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(toolTip));
            LOG_OUTPUT(L"ToolTip bounds left=%f top=%f width=%f height=%f at the end of slider", toolTipBounds.Left, toolTipBounds.Top, toolTipBounds.Width, toolTipBounds.Height);

            VERIFY_IS_TRUE(toolTipBounds.Left > sliderBounds.Left && toolTipBounds.Left + toolTipBounds.Width < sliderBounds.Left + sliderBounds.Width + toolTipBounds.Width);
            VERIFY_IS_TRUE(toolTipBounds.Top < sliderBounds.Top && toolTipBounds.Top > sliderBounds.Top - 50);
        });

        TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::ValidatePlacementWithoutValidHWnd()
    {
        TestCleanupWrapper cleanup;

        // This is regression coverage for:
        // "Settings App crashes when hovering over the different countries in the offline maps section"
        // This bug was hit when a ToolTip opened on top of another element (which had its own ToolTip).
        // If the mouse was moved to the open ToolTip, once that ToolTip closed and the ToolTip for the other element
        // behind it was opened, we would hit this crash.

        xaml::FrameworkElement^ targetElement1;
        xaml::FrameworkElement^ targetElement2;
        xaml_controls::ToolTip^ toolTip1;
        xaml_controls::ToolTip^ toolTip2;

        auto toolTip1OpenedEvent = std::make_shared<Event>();
        auto toolTip1OpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTip1ClosedEvent = std::make_shared<Event>();
        auto toolTip1ClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto toolTip2OpenedEvent = std::make_shared<Event>();
        auto toolTip2OpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTip2ClosedEvent = std::make_shared<Event>();
        auto toolTip2ClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <StackPanel Width="200" VerticalAlignment="Center" HorizontalAlignment="Center" >
                            <Border x:Name="targetElement2" Height="50" Background="Yellow" >
                                <ToolTipService.ToolTip>
                                    <ToolTip x:Name="toolTip2">
                                        ToolTip 2
                                    </ToolTip>
                                </ToolTipService.ToolTip>
                            </Border>
                            <Border x:Name="targetElement1" Height="50" Background="Green" >
                                <ToolTipService.ToolTip>
                                    <ToolTip x:Name="toolTip1">
                                        ToolTip 1
                                    </ToolTip>
                                </ToolTipService.ToolTip>
                            </Border>
                        </StackPanel>
                    </Grid>)")); // " SourceInsight cannot correctly parse R"()" string literals

            toolTip1 = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"toolTip1"));
            toolTip2 = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"toolTip2"));
            targetElement1 = dynamic_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"targetElement1"));
            targetElement2 = dynamic_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"targetElement2"));

            toolTip1OpenedRegistration.Attach(toolTip1, ref new xaml::RoutedEventHandler([toolTip1OpenedEvent](Platform::Object^, xaml::RoutedEventArgs^){
                toolTip1OpenedEvent->Set();
            }));
            toolTip1ClosedRegistration.Attach(toolTip1, ref new xaml::RoutedEventHandler([toolTip1ClosedEvent](Platform::Object^, xaml::RoutedEventArgs^){
                toolTip1ClosedEvent->Set();
            }));

            toolTip2OpenedRegistration.Attach(toolTip2, ref new xaml::RoutedEventHandler([toolTip2OpenedEvent](Platform::Object^, xaml::RoutedEventArgs^){
                toolTip2OpenedEvent->Set();
            }));
            toolTip2ClosedRegistration.Attach(toolTip2, ref new xaml::RoutedEventHandler([toolTip2ClosedEvent](Platform::Object^, xaml::RoutedEventArgs^){
                toolTip2ClosedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Show the first ToolTip:");
        auto targetElement1Bounds = ControlHelper::GetBounds(targetElement1);
        auto targetElement1Center = ControlHelper::GetCenterOfElement(targetElement1);
        wf::Point targetPoint = { targetElement1Center.X, targetElement1Bounds.Y + 5 };
        TestServices::InputHelper->MoveMouse(targetPoint);
        toolTip1OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // This test case only makes sense if toolTip1 opens on top of targetElement2:
        auto toolTip1Bounds = ControlHelper::GetBounds(toolTip1);
        auto targetElement2Bounds = ControlHelper::GetBounds(targetElement2);
        VERIFY_IS_TRUE(ControlHelper::IsContainedIn(toolTip1Bounds, targetElement2Bounds));

        LOG_OUTPUT(L"Show the second ToolTip:");
        // We move the mouse to the open toolTip which is on top of targetElement2.
        // Once toolTip1 closes, the mouse will be over targetElement2, and toolTip2 should open.
        targetPoint = ControlHelper::GetCenterOfElement(toolTip1);
        TestServices::InputHelper->MoveMouse(targetPoint);
        toolTip1ClosedEvent->WaitForDefault();
        toolTip2OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the second tooltip shows up in the correct place
        // We expect the tooltip to open such that:
        //  - Horizontally it is centered over the mouse position
        //  - Vertically the bottom of the tooltip is offset from the mouse position by ToolTipDefaultMouseOffset pixels
        auto toolTip2Center = ControlHelper::GetCenterOfElement(toolTip2);
        auto toolTip2Bounds = ControlHelper::GetBounds(toolTip2);
        auto expectedToolTip2BottomCenter = wf::Point{ targetPoint.X, targetPoint.Y - ToolTipDefaultMouseOffset };
        auto actualToolTip2BottomCenter = wf::Point{ toolTip2Center.X, toolTip2Bounds.Bottom };
        VERIFY_IS_TRUE(ControlHelper::AreClose(expectedToolTip2BottomCenter, actualToolTip2BottomCenter, 10));

        RunOnUIThread([&]()
        {
            toolTip2->IsOpen = false;
        });
        toolTip2ClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::CanSetToolTipOnNonStatefulObjects()
    {
        // Regression coverage for Threshold: Some UIElement types do not show ToolTip appied to them
        // As of Threshold, objects still need a framework peer to register for the ToolTip service.
        // When the peer reduction work went in, some types that didn't require peer state stopped creating
        // peers during parse, exposing a bug in the code that was suppoed to ensure a valid peer
        // when the ToolTip property was set.
        TestCleanupWrapper cleanup;

        std::vector<xaml::FrameworkElement^> elements;

        RunOnUIThread([&]
        {
            // This isn't an exhaustive list of types, but we don't really have a good way of iterating over
            // types and metadata and all that. But covering a few types is better than covering none.
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <Rectangle x:Name='tt1' Fill='Red' Width='200' Height='50'>"
                L"        <ToolTipService.ToolTip>"
                L"            <ToolTip>Rectangle ToolTip</ToolTip>"
                L"        </ToolTipService.ToolTip>"
                L"    </Rectangle>"
                L"    <Ellipse x:Name='tt2' Fill='Orange' Width='200' Height='50'>"
                L"        <ToolTipService.ToolTip>"
                L"            <ToolTip>Ellipse ToolTip</ToolTip>"
                L"        </ToolTipService.ToolTip>"
                L"    </Ellipse>"
                L"    <Border x:Name='tt3' Background='Yellow' Width='150' Height='50'>"
                L"        <ToolTipService.ToolTip>"
                L"            <ToolTip>Border ToolTip</ToolTip>"
                L"        </ToolTipService.ToolTip>"
                L"    </Border>"
                L"    <TextBlock x:Name='tt4' Text='TextBlock' FontSize='20'>"
                L"        <ToolTipService.ToolTip>"
                L"            <ToolTip>TextBlock ToolTip</ToolTip>"
                L"        </ToolTipService.ToolTip>"
                L"    </TextBlock>"
                L"</StackPanel>"));

            elements.emplace_back(safe_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"tt1")));
            elements.emplace_back(safe_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"tt2")));
            elements.emplace_back(safe_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"tt3")));
            elements.emplace_back(safe_cast<xaml::FrameworkElement^>(rootPanel->FindName(L"tt4")));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that each ToolTip opens by putting the mouse over its owner and waiting for the event
        for (auto element : elements)
        {
            xaml_controls::ToolTip^ toolTip = nullptr;

            auto ttOpenedEvent = std::make_shared<Event>();
            auto ttOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);

            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Testing popup on element: %s", element->Name->Data());
                toolTip = safe_cast<xaml_controls::ToolTip^>(xaml_controls::ToolTipService::GetToolTip(element));
                ttOpenedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([ttOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^) {
                    ttOpenedEvent->Set();
                }));
            });

            const auto targetBounds = ControlHelper::GetBounds(element);
            const wf::Point targetPoint = { targetBounds.X + (targetBounds.Width / 2), targetBounds.Y + (targetBounds.Height / 2) };
            TestServices::InputHelper->MoveMouse(targetPoint);
            ttOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                toolTip->IsOpen = false;
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void ToolTipIntegrationTests::CloseToolTipBeforeItIsFullyOpen()
    {
        // This is regression coverage for "Excel: Font and Fill color ToolTip sometimes gets stuck in Open state"
        // Prior to this bug fix, it was possible to get ToolTip into a state where its Popup remained Open even when the ToolTip
        // was in the Closed state.
        // This could happen by setting the IsOpen property back to false before it had time to fully complete opening.
        TestCleanupWrapper cleanup;

        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();
        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            toolTip->IsOpen = true;
            toolTip->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        // We expect there to be no open popups on the screen.
        RunOnUIThread([&]()
        {
            auto currentlyOpenPopups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_ARE_EQUAL(0u, currentlyOpenPopups->Size);
        });
    }

    void ToolTipIntegrationTests::CanEnableAndDisableOpenedToolTip()
    {
        TestCleanupWrapper cleanup;

        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();
        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            toolTip->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // ToolTip is enabled by default.
            VERIFY_IS_TRUE(toolTip->IsOpen);
            VERIFY_IS_TRUE(toolTip->IsEnabled);
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_ARE_EQUAL(1u, popups->Size);
            auto toolTipParentPopup = popups->GetAt(0);
            VERIFY_ARE_EQUAL(1, toolTipParentPopup->Opacity);

            toolTip->IsEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Disabling the ToolTip causes the ToolTip to disappear.
            VERIFY_IS_TRUE(toolTip->IsOpen);
            VERIFY_IS_FALSE(toolTip->IsEnabled);
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_ARE_EQUAL(1u, popups->Size);
            auto toolTipParentPopup = popups->GetAt(0);
            VERIFY_ARE_EQUAL(0, toolTipParentPopup->Opacity);
        });
        // TODO: Figure out why we can't enable an opened ToolTip.
        /*
        RunOnUIThread([&]()
        {
            toolTip->IsEnabled = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Enabling the ToolTip causes the ToolTip to appear.
            VERIFY_IS_TRUE(toolTip->IsOpen);
            VERIFY_IS_TRUE(toolTip->IsEnabled);
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_ARE_EQUAL(1u, popups->Size);
            VERIFY_ARE_EQUAL(1, toolTip->Opacity);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, toolTip->Visibility);
        });*/
        TestServices::WindowHelper->WaitForIdle();

        CloseToolTip(toolTip, InputMode::None);
    }

    void ToolTipIntegrationTests::CanOpenAndCloseDisabledToolTip()
    {
        TestCleanupWrapper cleanup;

        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();
        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
            toolTip->IsEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // ToolTip is closed by default.
            VERIFY_IS_FALSE(toolTip->IsOpen);
            VERIFY_IS_FALSE(toolTip->IsEnabled);
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_ARE_EQUAL(0u, popups->Size);

            toolTip->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Setting IsOpen to true on a disabled ToolTip does not make it appear.
            VERIFY_IS_TRUE(toolTip->IsOpen);
            VERIFY_IS_FALSE(toolTip->IsEnabled);
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_ARE_EQUAL(1u, popups->Size);
            auto toolTipParentPopup = popups->GetAt(0);
            VERIFY_ARE_EQUAL(0, toolTipParentPopup->Opacity);

            toolTip->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Setting IsOpen to false, closes the parent Popup.
            VERIFY_IS_FALSE(toolTip->IsOpen);
            VERIFY_IS_FALSE(toolTip->IsEnabled);
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_ARE_EQUAL(0u, popups->Size);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::VerifyPlacementTargetIsHonored()
    {
        TestCleanupWrapper cleanup;

        auto toolTip = CreateToolTip();

        xaml_controls::Button^ topButton = nullptr;
        xaml_controls::Button^ bottomButton = nullptr;

        wf::Rect toolTipBoundsFromTopButton;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            openedRegistration.Attach(toolTip, [openedEvent]() { openedEvent->Set(); });
            closedRegistration.Attach(toolTip, [closedEvent]() { closedEvent->Set(); });

            auto rootPanel = ref new xaml_controls::StackPanel();

            // Center-align the root panel to ensure that both tooltips appear in the same
            // relative position to their targets.
            rootPanel->VerticalAlignment = xaml::VerticalAlignment::Center;

            topButton = ref new xaml_controls::Button();
            topButton->Content = L"Button.ToolTip";
            topButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;

            rootPanel->Children->Append(topButton);

            bottomButton = ref new xaml_controls::Button();
            bottomButton->Content = L"Button.ToolTipPlacementTarget";
            bottomButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;

            rootPanel->Children->Append(bottomButton);

            xaml_controls::ToolTipService::SetToolTip(topButton, toolTip);

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            toolTip->IsOpen = true;
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            toolTipBoundsFromTopButton = ControlHelper::GetBounds(toolTip);
            toolTip->IsOpen = false;
        });

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetPlacementTarget(topButton, bottomButton);
            toolTip->IsOpen = true;
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto toolTipBoundsFromBottomButton = ControlHelper::GetBounds(toolTip);

            VERIFY_ARE_EQUAL(toolTipBoundsFromTopButton.X, toolTipBoundsFromBottomButton.X);
            VERIFY_IS_LESS_THAN(toolTipBoundsFromTopButton.Y, toolTipBoundsFromBottomButton.Y);

            toolTip->IsOpen = false;
        });

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::CanOpenToolTipWithKeyboardFocusWithMouse()
    {
        CanOpenToolTipWithKeyboardFocus(true);
    }

    void ToolTipIntegrationTests::CanOpenToolTipWithKeyboardFocusWithoutMouse()
    {
        CanOpenToolTipWithKeyboardFocus(false);
    }

    void ToolTipIntegrationTests::CanOpenToolTipWithKeyboardFocus(bool withMouse)
    {
        TestCleanupWrapper cleanup;

        auto toolTip = CreateToolTip();

        xaml_controls::Button^ topButton = nullptr;
        xaml_controls::Button^ bottomButton = nullptr;

        wf::Rect toolTipBoundsFromMouseHover;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::StackPanel();

            topButton = ref new xaml_controls::Button();
            topButton->Content = L"Button.Focus";
            topButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;

            rootPanel->Children->Append(topButton);

            bottomButton = ref new xaml_controls::Button();
            bottomButton->Content = L"Button.ToolTip";
            bottomButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;

            rootPanel->Children->Append(bottomButton);

            xaml_controls::ToolTipService::SetToolTip(bottomButton, toolTip);

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;

            rootPanel->Focus(FocusState::Keyboard);
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Opening Bottom");
        OpenToolTip(bottomButton, toolTip, withMouse ? InputMode::Mouse : InputMode::None, false /*isTargetPosition*/, wf::Point(0, 0));

        RunOnUIThread([&]()
        {
            toolTipBoundsFromMouseHover = ControlHelper::GetBounds(toolTip);
        });

        CloseToolTip(toolTip, withMouse ? InputMode::Mouse : InputMode::None);

        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            openedRegistration.Attach(toolTip, [openedEvent]() { openedEvent->Set(); });
            closedRegistration.Attach(toolTip, [closedEvent]() { closedEvent->Set(); });

            topButton->Focus(xaml::FocusState::Keyboard);
        });

        TestServices::KeyboardHelper->Tab();

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto toolTipBoundsFromKeyboardFocus = ControlHelper::GetBounds(toolTip);

            //The InputMode::None method of opening the tooltip defaults to a top position, which is much very different from the other
            //methods tested here, so we apply a larger tolerance.
            LOG_OUTPUT(L"ToolTip position from mouse hover:    (%.2f, %.2f)", toolTipBoundsFromMouseHover.X, toolTipBoundsFromMouseHover.Y);
            LOG_OUTPUT(L"ToolTip position from keyboard focus: (%.2f, %.2f)", toolTipBoundsFromKeyboardFocus.X, toolTipBoundsFromKeyboardFocus.Y);
            LOG_OUTPUT(L"The positions are expected to be somewhat offset since we position the ToolTip differently based on the input used.");
            if (withMouse)
            {
                LOG_OUTPUT(L"We'll apply a tolerance of 10 pixels for the two positions to be considered valid.");
                VERIFY_IS_TRUE(abs(toolTipBoundsFromMouseHover.X - toolTipBoundsFromKeyboardFocus.X) <= 10);
                VERIFY_IS_TRUE(abs(toolTipBoundsFromMouseHover.Y - toolTipBoundsFromKeyboardFocus.Y) <= 10);
            }
            else
            {
                LOG_OUTPUT(L"We'll apply a tolerance of 100 pixels for the two positions to be considered valid.");
                VERIFY_IS_TRUE(abs(toolTipBoundsFromMouseHover.X - toolTipBoundsFromKeyboardFocus.X) <= 100);
                VERIFY_IS_TRUE(abs(toolTipBoundsFromMouseHover.Y - toolTipBoundsFromKeyboardFocus.Y) <= 100);
            }
        });

        TestServices::KeyboardHelper->Tab();

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::VerifyToolTipPlacedCorrectlyWhenRTL()
    {
        TestCleanupWrapper cleanup;

        auto toolTip = CreateToolTip();

        xaml_controls::Button^ button = nullptr;

        wf::Rect toolTipBoundsFromLTR;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::StackPanel();

            rootPanel->VerticalAlignment = xaml::VerticalAlignment::Center;

            button = ref new xaml_controls::Button();
            button->Content = L"Button.ToolTip";
            button->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            button->Margin = xaml::Thickness({ TestServices::WindowHelper->VisibleBounds.X, TestServices::WindowHelper->VisibleBounds.Y, 0, 0 });

            rootPanel->Children->Append(button);

            xaml_controls::ToolTipService::SetToolTip(button, toolTip);

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        OpenToolTip(button, toolTip, InputMode::Touch, false /*isTargetPosition*/, wf::Point(0, 0));

        RunOnUIThread([&]()
        {
            toolTipBoundsFromLTR = ControlHelper::GetBounds(toolTip);
        });

        CloseToolTip(toolTip, InputMode::Touch);

        RunOnUIThread([&]()
        {
            button->FlowDirection = xaml::FlowDirection::RightToLeft;
        });

        OpenToolTip(button, toolTip, InputMode::Touch, false /*isTargetPosition*/, wf::Point(0, 0));

        RunOnUIThread([&]()
        {
            auto toolTipBoundsFromRTL = ControlHelper::GetBounds(toolTip);

            VERIFY_ARE_EQUAL(toolTipBoundsFromLTR.X, toolTipBoundsFromRTL.X);
            VERIFY_ARE_EQUAL(toolTipBoundsFromLTR.Y, toolTipBoundsFromRTL.Y);
        });

        CloseToolTip(toolTip, InputMode::Touch);
    }

    void ToolTipIntegrationTests::VerifyToolTipAlwaysInViewWithMouse()
    {
        VerifyToolTipAlwaysInView(true);
    }

    void ToolTipIntegrationTests::VerifyToolTipAlwaysInViewWithoutMouse()
    {
        VerifyToolTipAlwaysInView(false);
    }

    void ToolTipIntegrationTests::VerifyToolTipAlwaysInView(bool withMouse)
    {
        TestCleanupWrapper cleanup;

        auto rightToolTip = CreateToolTip();
        auto bottomToolTip = CreateToolTip();

        xaml_controls::Button^ rightButton = nullptr;
        xaml_controls::Button^ bottomButton = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();

            rightButton = ref new xaml_controls::Button();
            rightButton->Content = L"R";
            rightButton->HorizontalAlignment = xaml::HorizontalAlignment::Right;
            rightButton->VerticalAlignment = xaml::VerticalAlignment::Center;

            rootPanel->Children->Append(rightButton);

            bottomButton = ref new xaml_controls::Button();
            bottomButton->Content = L"B";
            bottomButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            bottomButton->VerticalAlignment = xaml::VerticalAlignment::Bottom;

            rootPanel->Children->Append(bottomButton);

            rightToolTip->Placement = xaml_primitives::PlacementMode::Mouse;
            bottomToolTip->Placement = xaml_primitives::PlacementMode::Mouse;

            xaml_controls::ToolTipService::SetToolTip(rightButton, rightToolTip);
            xaml_controls::ToolTipService::SetToolTip(bottomButton, bottomToolTip);

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        OpenToolTip(rightButton, rightToolTip, withMouse ? InputMode::Mouse : InputMode::None, false /*isTargetPosition*/, wf::Point(0, 0));

        RunOnUIThread([&]()
        {
            auto toolTipBounds = ControlHelper::GetBounds(rightToolTip);

            auto xamlRoot = rightToolTip->XamlRoot;
            VERIFY_IS_NOT_NULL(xamlRoot);
            auto xamlrootSize = xamlRoot->Size;

            VERIFY_IS_LESS_THAN_OR_EQUAL(toolTipBounds.X + toolTipBounds.Width, xamlrootSize.Width);
        });

        CloseToolTip(rightToolTip, withMouse ? InputMode::Mouse : InputMode::None);

        OpenToolTip(bottomButton, bottomToolTip, withMouse ? InputMode::Mouse : InputMode::None, false /*isTargetPosition*/, wf::Point(0, 0));

        RunOnUIThread([&]()
        {
            auto toolTipBounds = ControlHelper::GetBounds(bottomToolTip);

            auto xamlRoot = bottomToolTip->XamlRoot;
            VERIFY_IS_NOT_NULL(xamlRoot);
            auto xamlRootSize = xamlRoot->Size;

            VERIFY_IS_LESS_THAN_OR_EQUAL(toolTipBounds.Y + toolTipBounds.Height, xamlRootSize.Height);
        });

        CloseToolTip(bottomToolTip, withMouse ? InputMode::Mouse : InputMode::None);
    }

    void ToolTipIntegrationTests::VerifyToolTipClippedIfLargerThanWindowWithMouse()
    {
        VerifyToolTipClippedIfLargerThanWindow(true);
    }

    void ToolTipIntegrationTests::VerifyToolTipClippedIfLargerThanWindowWithoutMouse()
    {
        VerifyToolTipClippedIfLargerThanWindow(false);
    }

    void ToolTipIntegrationTests::VerifyToolTipClippedIfLargerThanWindow(bool withMouse)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        auto toolTip = CreateTallToolTip();

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();

            button = ref new xaml_controls::Button();
            button->Content = L"Button.ToolTip";
            button->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            button->VerticalAlignment = xaml::VerticalAlignment::Center;

            rootPanel->Children->Append(button);

            toolTip->Placement = xaml_primitives::PlacementMode::Mouse;
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        OpenToolTip(button, toolTip, withMouse ? InputMode::Mouse : InputMode::None, false /*isTargetPosition*/, wf::Point(0, 0));

        wf::Rect windowBounds = {};
        RunOnUIThread([&]()
        {
            windowBounds = TestServices::WindowHelper->WindowBounds;
            LOG_OUTPUT(L"Windows bounds left=%f top=%f width=%f height=%f", windowBounds.Left, windowBounds.Top, windowBounds.Width, windowBounds.Height);
            LOG_OUTPUT(L"ToolTip height=%f, content height=%f)", toolTip->ActualHeight, safe_cast<xaml_controls::TextBlock^>(toolTip->Content)->ActualHeight);
            VERIFY_IS_LESS_THAN(toolTip->ActualHeight, safe_cast<xaml_controls::TextBlock^>(toolTip->Content)->ActualHeight);
        });

        CloseToolTip(toolTip, withMouse ? InputMode::Mouse : InputMode::None);
    }

    void ToolTipIntegrationTests::VerifyToolTipIsRespositionedOnRootVisualSizeChange()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Button^ topButton = nullptr;
        xaml_controls::Button^ bottomButton = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        wf::Rect toolTipOriginalBounds;

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            topButton = ref new xaml_controls::Button();
            topButton->Content = L"Button.Focus";
            topButton->HorizontalAlignment = xaml::HorizontalAlignment::Left;

            rootPanel->Children->Append(topButton);

            bottomButton = ref new xaml_controls::Button();
            bottomButton->Content = L"Button.ToolTip";
            bottomButton->HorizontalAlignment = xaml::HorizontalAlignment::Left;

            rootPanel->Children->Append(bottomButton);

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // This needs to occur *after* the call to set WindowContent, since otherwise
        // RootVisual will be null and we won't properly hook up to it for the purposes
        // of this test.
        auto toolTip = CreateToolTip();

        RunOnUIThread([&]()
        {
            toolTip->Placement = xaml_primitives::PlacementMode::Mouse;
            xaml_controls::ToolTipService::SetToolTip(bottomButton, toolTip);

            openedRegistration.Attach(toolTip, [openedEvent]() { openedEvent->Set(); });
            closedRegistration.Attach(toolTip, [closedEvent]() { closedEvent->Set(); });

            topButton->Focus(xaml::FocusState::Keyboard);
        });

        TestServices::KeyboardHelper->Tab();

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            toolTipOriginalBounds = ControlHelper::GetBounds(toolTip);
        });

        auto sizeChangedEvent = std::make_shared<Event>();
        auto sizeChangedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, SizeChanged);

        RunOnUIThread([&]()
        {
            sizeChangedRegistration.Attach(rootPanel, [sizeChangedEvent]() { sizeChangedEvent->Set(); });

            // We'll shrink the size of the root panel by 200 pixels, an arbitrary number
            // just so that we can test whether the ToolTip moves after we do this.
            rootPanel->Width = rootPanel->ActualWidth - 200;
        });

        sizeChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto toolTipNewBounds = ControlHelper::GetBounds(toolTip);

            VERIFY_IS_LESS_THAN(toolTipOriginalBounds.X, toolTipNewBounds.X);
        });

        TestServices::KeyboardHelper->Tab();

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::ShowToolTipFromWindowedPopup()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestCleanupWrapper cleanup;

        // Regression coverage for:
        // "Tooltips are mis-positioned when attached to items within a menu item"
        //
        // The problem is with the fact that MenuFlyout is shown in a windowed popup. Since the ToolTip Popup is positioned
        // relative to the Jupiter window, when showing a ToolTip from a different window (e.g. the MenuFlyout) we need
        // to ensure that the ToolTip Popup position is transformed appropriately.

        xaml_controls::Grid^ rootPanel;
        xaml_controls::MenuFlyout^ menuFlyout;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem;
        xaml_controls::ToolTip^ toolTip;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                            xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <Grid.Resources>
                            <MenuFlyout x:Name="menuFlyout">
                                <MenuFlyoutItem x:Name="menuFlyoutItem" Text="MenuFlyoutItem" Height="150" >
                                    <ToolTipService.ToolTip>
                                        <ToolTip x:Name="toolTip">
                                            <TextBlock Text="ToolTip" />
                                        </ToolTip>
                                    </ToolTipService.ToolTip>
                                </MenuFlyoutItem>
                            </MenuFlyout>
                        </Grid.Resources>
                    </Grid>)"));    //" - SourceInsight does not parse multi-line string literals correctly

            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(rootPanel->FindName(L"menuFlyout"));
            menuFlyoutItem = safe_cast<xaml_controls::MenuFlyoutItem^>(rootPanel->FindName(L"menuFlyoutItem"));
            toolTip = safe_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"toolTip"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        //TestServices::InputHelper->MoveMouse(wf::Point(0, 0));
        test_infra::TestServices::InputHelper->LeftMouseClick(rootPanel);
        TestServices::WindowHelper->WaitForIdle();

        // Open the MenuFlyout:
        {
            Event menuFlyoutOpenedEvent;
            auto menuFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
            menuFlyoutOpenedRegistration.Attach(menuFlyout, [&]() { menuFlyoutOpenedEvent.Set(); });

            RunOnUIThread([&]()
            {
                menuFlyout->ShowAt(rootPanel, wf::Point(200, 200));
            });
            TestServices::WindowHelper->WaitForIdle();
            menuFlyoutOpenedEvent.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        // Show the ToolTip by hovering over the MenuFlyoutItem:
        {
            Event toolTipOpenedEvent;
            auto tooltipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
            tooltipOpenedRegistration.Attach(toolTip, [&]() { toolTipOpenedEvent.Set(); });

            TestServices::InputHelper->MoveMouse(menuFlyoutItem);
            TestServices::WindowHelper->WaitForIdle();
            toolTipOpenedEvent.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        // For this test case it is unnecessary to verify the exact position of the ToolTip.
        // Verifying that it is within the bounds of the MenuFlyoutItem is sufficient to
        // prevent this bug from regressing.
        auto toolTipBounds = ControlHelper::GetBounds(toolTip);
        auto menuFlyoutItemBounds = ControlHelper::GetBounds(menuFlyoutItem);
        VERIFY_IS_TRUE(ControlHelper::IsContainedIn(toolTipBounds, menuFlyoutItemBounds));

        FlyoutHelper::HideFlyout(menuFlyout);
    }

    void ToolTipIntegrationTests::VerifyToolTipOnlyShowsProgramaticallyOnXbox()
    {
        TestCleanupWrapper cleanup;
        int timeoutMS = 2000;

        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();
        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        openedRegistration.Attach(toolTip, [&](){ openedEvent->Set(); });

        // ToolTip should NOT open due to target getting keyboard focus:
        RunOnUIThread([&]()
        {
            button->Focus(xaml::FocusState::Keyboard);
        });
        VERIFY_IS_FALSE(openedEvent->WaitForNoThrow(std::chrono::milliseconds(timeoutMS)));

        // ToolTip should NOT open due to Mouse hovering target:
        TestServices::InputHelper->MoveMouse(button);
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(openedEvent->WaitForNoThrow(std::chrono::milliseconds(timeoutMS)));

        // ToolTip should NOT open due to press and hold on target:
        TestServices::InputHelper->Hold(button, timeoutMS);
        VERIFY_IS_FALSE(openedEvent->HasFired());

        // ToolTip SHOULD display when opened programatically
        PerformToolTipPlacement(button, toolTip, xaml_primitives::PlacementMode::Top, xaml_primitives::PlacementMode::Top, InputMode::None);
    }

    void ToolTipIntegrationTests::ValidateTooltipIsNotTopmost()
    {
        ValidateTooltipIsTopmostHelper(false /* componentHosted*/);
    }

    void ToolTipIntegrationTests::ValidateTooltipIsTopmostHelper(bool componentHosted)
    {
        TestCleanupWrapper cleanup;

        auto button = SetupToolTipTest();
        auto toolTip = CreateToolTip();

        RunOnUIThread([&]()
        {
            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
        });

        OpenToolTip(button, toolTip, InputMode::Mouse, false /*isTargetPosition*/, wf::Point(0, 0));

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);

            LOG_OUTPUT(L"Validating whether tooltip %s topmost ...", componentHosted ? L"is" : L"is not");
            HWND toolTipHwnd = (HWND)(TestServices::WindowHelper->ToolTip_GetWindow(toolTip));
            DWORD dwExStyle = ::GetWindowLong(toolTipHwnd, GWL_EXSTYLE);
            bool isTopmost = (dwExStyle & WS_EX_TOPMOST) != 0;

            // isTopmost should only be set for tooltip hosted as component.
            VERIFY_IS_TRUE(isTopmost == componentHosted);
        });

        CloseToolTip(toolTip, InputMode::Mouse);
    }

    void ToolTipIntegrationTests::ValidateTooltipRepositionsAfterWindowPositionChange()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ button = nullptr;
        wf::Rect originalToolTipBounds = {};

        auto toolTip = CreateToolTip();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button x:Name='button' Content='Open tool tip!' HorizontalAlignment='Left' VerticalAlignment='Top' ToolTipService.Placement='Left'/>"
                L"</Grid>"));

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            xaml_controls::ToolTipService::SetToolTip(button, toolTip);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        OpenToolTip(button, toolTip, InputMode::None, false /* isTargetPosition */, wf::Point(0, 0));

        originalToolTipBounds = GetToolTipBounds(toolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Original tool tip position: (%lf, %lf)", originalToolTipBounds.X, originalToolTipBounds.Y);
        });

        // SetDesktopWindowSize un-maximizes the window, which gets us the WindowPositionChange event we want.
        TestServices::WindowHelper->SetDesktopWindowSize(400, 400);
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect newToolTipBounds = GetToolTipBounds(toolTip);

        RunOnUIThread([&]()
        {
            // The tool tip should now have been moved along with the window contents.
            LOG_OUTPUT(L"New tool tip position:      (%lf, %lf)", newToolTipBounds.X, newToolTipBounds.Y);

            VERIFY_ARE_NOT_EQUAL(newToolTipBounds.X, originalToolTipBounds.X);
            VERIFY_ARE_NOT_EQUAL(newToolTipBounds.Y, originalToolTipBounds.Y);
        });

        CloseToolTip(toolTip, InputMode::None);
    }

    void ToolTipIntegrationTests::ValidatePlacementRectWithKeyboard()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ initialFocusButton = nullptr;
        xaml_controls::ToolTip^ regularToolTip = nullptr;
        xaml_controls::ToolTip^ wideMarginToolTip = nullptr;
        xaml_controls::ToolTip^ thinMarginToolTip = nullptr;

        auto regularToolTipOpenedEvent = std::make_shared<Event>();
        auto regularToolTipClosedEvent = std::make_shared<Event>();
        auto wideMarginToolTipOpenedEvent = std::make_shared<Event>();
        auto wideMarginToolTipClosedEvent = std::make_shared<Event>();
        auto thinMarginToolTipOpenedEvent = std::make_shared<Event>();
        auto thinMarginToolTipClosedEvent = std::make_shared<Event>();
        auto regularToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto regularToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto wideMarginToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto wideMarginToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto thinMarginToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto thinMarginToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel Orientation="Horizontal" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name="InitialFocusButton" Content="Initial focus button" Margin="20" Width="100" Height="100" />
                        <Button Content="Regular ToolTip" Margin="20" Width="100" Height="100">
                            <ToolTipService.ToolTip>
                                <ToolTip Content="ToolTip" x:Name="RegularToolTip" />
                            </ToolTipService.ToolTip>
                        </Button>
                        <Button Content="ToolTip with wide margins" Margin="20" Width="100" Height="100">
                            <ToolTipService.ToolTip>
                                <ToolTip Content="ToolTip" x:Name="WideMarginToolTip" PlacementRect="-50, -50, 200, 200" />
                            </ToolTipService.ToolTip>
                        </Button>
                        <Button Content="ToolTip with thin margins" Margin="20" Width="100" Height="100">
                            <ToolTipService.ToolTip>
                                <ToolTip Content="ToolTip" x:Name="ThinMarginToolTip" PlacementRect="40, 40, 20, 20" />
                            </ToolTipService.ToolTip>
                        </Button>
                        <Button Content="Final focus button" Margin="20" Width="100" Height="100" />
                    </StackPanel>)"));

            initialFocusButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"InitialFocusButton"));

            regularToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"RegularToolTip"));
            wideMarginToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"WideMarginToolTip"));
            thinMarginToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"ThinMarginToolTip"));

            regularToolTipOpenedRegistration.Attach(regularToolTip, [regularToolTipOpenedEvent]() { regularToolTipOpenedEvent->Set(); });
            regularToolTipClosedRegistration.Attach(regularToolTip, [regularToolTipClosedEvent]() { regularToolTipClosedEvent->Set(); });
            wideMarginToolTipOpenedRegistration.Attach(wideMarginToolTip, [wideMarginToolTipOpenedEvent]() { wideMarginToolTipOpenedEvent->Set(); });
            wideMarginToolTipClosedRegistration.Attach(wideMarginToolTip, [wideMarginToolTipClosedEvent]() { wideMarginToolTipClosedEvent->Set(); });
            thinMarginToolTipOpenedRegistration.Attach(thinMarginToolTip, [thinMarginToolTipOpenedEvent]() { thinMarginToolTipOpenedEvent->Set(); });
            thinMarginToolTipClosedRegistration.Attach(thinMarginToolTip, [thinMarginToolTipClosedEvent]() { thinMarginToolTipClosedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(initialFocusButton, FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Give focus to the regular tool tip button to open its tool tip.");
        TestServices::KeyboardHelper->Tab();
        regularToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect regularToolTipBounds = GetToolTipBounds(regularToolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Regular tool tip position: (%lf, %lf)", regularToolTipBounds.X, regularToolTipBounds.Y);
        });

        LOG_OUTPUT(L"Give focus to the wide-margin tool tip button to open its tool tip.");
        TestServices::KeyboardHelper->Tab();
        regularToolTipClosedEvent->WaitForDefault();
        wideMarginToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect wideMarginToolTipBounds = GetToolTipBounds(wideMarginToolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Wide-margin tool tip position: (%lf, %lf)", wideMarginToolTipBounds.X, wideMarginToolTipBounds.Y);
            VERIFY_ARE_EQUAL(regularToolTipBounds.Y - 50, wideMarginToolTipBounds.Y);
        });

        LOG_OUTPUT(L"Give focus to the thin-margin tool tip button to open its tool tip.");
        TestServices::KeyboardHelper->Tab();
        wideMarginToolTipClosedEvent->WaitForDefault();
        thinMarginToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect thinMarginToolTipBounds = GetToolTipBounds(thinMarginToolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Thin-margin tool tip position: (%lf, %lf)", thinMarginToolTipBounds.X, thinMarginToolTipBounds.Y);
            VERIFY_ARE_EQUAL(regularToolTipBounds.Y + 40, thinMarginToolTipBounds.Y);
        });

        LOG_OUTPUT(L"Give focus to the final button to close the tool tip.");
        TestServices::KeyboardHelper->Tab();
        thinMarginToolTipClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::ValidatePlacementRectWithMouse()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ regularToolTipButton = nullptr;
        xaml_controls::Button^ wideMarginToolTipButton = nullptr;
        xaml_controls::Button^ thinMarginToolTipButton = nullptr;

        xaml_controls::ToolTip^ regularToolTip = nullptr;
        xaml_controls::ToolTip^ wideMarginToolTip = nullptr;
        xaml_controls::ToolTip^ thinMarginToolTip = nullptr;

        auto regularToolTipOpenedEvent = std::make_shared<Event>();
        auto regularToolTipClosedEvent = std::make_shared<Event>();
        auto wideMarginToolTipOpenedEvent = std::make_shared<Event>();
        auto wideMarginToolTipClosedEvent = std::make_shared<Event>();
        auto thinMarginToolTipOpenedEvent = std::make_shared<Event>();
        auto thinMarginToolTipClosedEvent = std::make_shared<Event>();
        auto regularToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto regularToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto wideMarginToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto wideMarginToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto thinMarginToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto thinMarginToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel Orientation="Horizontal" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name="RegularToolTipButton" Content="Regular ToolTip" Margin="20" Width="100" Height="100">
                            <ToolTipService.ToolTip>
                                <ToolTip Content="ToolTip" x:Name="RegularToolTip" />
                            </ToolTipService.ToolTip>
                        </Button>
                        <Button x:Name="WideMarginToolTipButton" Content="ToolTip with wide margins" Margin="20" Width="100" Height="100">
                            <ToolTipService.ToolTip>
                                <ToolTip Content="ToolTip" x:Name="WideMarginToolTip" PlacementRect="-50, -50, 200, 200" />
                            </ToolTipService.ToolTip>
                        </Button>
                        <Button x:Name="ThinMarginToolTipButton" Content="ToolTip with thin margins" Margin="20" Width="100" Height="100">
                            <ToolTipService.ToolTip>
                                <ToolTip Content="ToolTip" x:Name="ThinMarginToolTip" PlacementRect="40, 40, 20, 20" />
                            </ToolTipService.ToolTip>
                        </Button>
                    </StackPanel>)"));

            regularToolTipButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"RegularToolTipButton"));
            wideMarginToolTipButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"WideMarginToolTipButton"));
            thinMarginToolTipButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"ThinMarginToolTipButton"));

            regularToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"RegularToolTip"));
            wideMarginToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"WideMarginToolTip"));
            thinMarginToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"ThinMarginToolTip"));

            regularToolTipOpenedRegistration.Attach(regularToolTip, [regularToolTipOpenedEvent]() { regularToolTipOpenedEvent->Set(); });
            regularToolTipClosedRegistration.Attach(regularToolTip, [regularToolTipClosedEvent]() { regularToolTipClosedEvent->Set(); });
            wideMarginToolTipOpenedRegistration.Attach(wideMarginToolTip, [wideMarginToolTipOpenedEvent]() { wideMarginToolTipOpenedEvent->Set(); });
            wideMarginToolTipClosedRegistration.Attach(wideMarginToolTip, [wideMarginToolTipClosedEvent]() { wideMarginToolTipClosedEvent->Set(); });
            thinMarginToolTipOpenedRegistration.Attach(thinMarginToolTip, [thinMarginToolTipOpenedEvent]() { thinMarginToolTipOpenedEvent->Set(); });
            thinMarginToolTipClosedRegistration.Attach(thinMarginToolTip, [thinMarginToolTipClosedEvent]() { thinMarginToolTipClosedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->MoveMouse(wf::Point(0, 0));
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Move the mouse over the regular tool tip button to open its tool tip.");
        TestServices::InputHelper->MoveMouse(regularToolTipButton);
        regularToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect regularToolTipBounds = GetToolTipBounds(regularToolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Regular tool tip position: (%lf, %lf)", regularToolTipBounds.X, regularToolTipBounds.Y);
        });

        LOG_OUTPUT(L"Move the mouse over the wide-margin tool tip button to open its tool tip.");
        TestServices::InputHelper->MoveMouse(wideMarginToolTipButton);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Wait for regular tool tip close event.");
        regularToolTipClosedEvent->WaitForDefault();

        LOG_OUTPUT(L"Wait for wide-margin tool tip open event.");
        wideMarginToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect wideMarginToolTipBounds = GetToolTipBounds(wideMarginToolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Wide-margin tool tip position: (%lf, %lf)", wideMarginToolTipBounds.X, wideMarginToolTipBounds.Y);

            // wideMarginToolTipBounds.top should be 100 pixels higher than regularToolTipBounds.top,
            // but integer truncation along the way can cause it to be slightly off from that.
            VERIFY_IS_TRUE(abs((regularToolTipBounds.Y - 100) - wideMarginToolTipBounds.Y < 5));
        });

        LOG_OUTPUT(L"Move the mouse over the thin-margin tool tip button to open its tool tip.");
        TestServices::InputHelper->MoveMouse(thinMarginToolTipButton);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Wait for wide-margin tool tip close event.");
        wideMarginToolTipClosedEvent->WaitForDefault();

        LOG_OUTPUT(L"Wait for thin-margin tool tip open event.");
        thinMarginToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect thinMarginToolTipBounds = GetToolTipBounds(thinMarginToolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Thin-margin tool tip position: (%lf, %lf)", thinMarginToolTipBounds.X, thinMarginToolTipBounds.Y);

            // toolTipBounds.top should be 10 pixels higher than regularToolTipBounds.top,
            // but integer truncation along the way can cause it to be slightly off from that.
            VERIFY_IS_TRUE(abs((regularToolTipBounds.Y - 10) - thinMarginToolTipBounds.Y < 5));
        });

        LOG_OUTPUT(L"Move the mouse back to the top-left corner to close the tool tip.");
        TestServices::InputHelper->MoveMouse(wf::Point(20, 20));
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Wait for thin-margin tool tip close event.");
        thinMarginToolTipClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::ValidatePlacementRectChangeInSizeChanged()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ initialFocusButton = nullptr;
        xaml_controls::ToolTip^ regularToolTip = nullptr;
        xaml_controls::ToolTip^ placedToolTip = nullptr;

        auto regularToolTipOpenedEvent = std::make_shared<Event>();
        auto regularToolTipClosedEvent = std::make_shared<Event>();
        auto placedToolTipOpenedEvent = std::make_shared<Event>();
        auto placedToolTipClosedEvent = std::make_shared<Event>();
        auto placedToolTipSizeChangedEvent = std::make_shared<Event>();

        auto regularToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto regularToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto placedToolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto placedToolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);
        auto placedToolTipSizeChangedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, SizeChanged);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel Orientation="Horizontal" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name="initialFocusButton" Content="Initial focus button" Margin="20" Width="100" Height="100" />
                        <Button Content="Regular ToolTip" Margin="20" Width="100" Height="100">
                            <ToolTipService.ToolTip>
                                <ToolTip x:Name="regularToolTip" Content="ToolTip"/>
                            </ToolTipService.ToolTip>
                        </Button>
                        <Button Content="ToolTip with PlacementRect" Margin="20" Width="100" Height="100">
                            <ToolTipService.ToolTip>
                                <ToolTip x:Name="placedToolTip" Content="ToolTip"/>
                            </ToolTipService.ToolTip>
                        </Button>
                        <Button Content="Final focus button" Margin="20" Width="100" Height="100" />
                    </StackPanel>)"));

            initialFocusButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"initialFocusButton"));
            regularToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"regularToolTip"));
            placedToolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"placedToolTip"));

            regularToolTipOpenedRegistration.Attach(regularToolTip, [regularToolTipOpenedEvent]()
            {
                LOG_OUTPUT(L"regularToolTip.Opened event raised.");
                regularToolTipOpenedEvent->Set();
            });

            regularToolTipClosedRegistration.Attach(regularToolTip, [regularToolTipClosedEvent]()
            {
                LOG_OUTPUT(L"regularToolTip.Closed event raised.");
                regularToolTipClosedEvent->Set();
            });

            placedToolTipOpenedRegistration.Attach(placedToolTip, [placedToolTipOpenedEvent]()
            {
                LOG_OUTPUT(L"placedToolTip.Opened event raised.");
                placedToolTipOpenedEvent->Set();
            });

            placedToolTipClosedRegistration.Attach(placedToolTip, [placedToolTipClosedEvent]()
            {
                LOG_OUTPUT(L"placedToolTip.Closed event raised.");
                placedToolTipClosedEvent->Set();
            });

            placedToolTipSizeChangedRegistration.Attach(placedToolTip, [placedToolTip, placedToolTipSizeChangedEvent]()
            {
                LOG_OUTPUT(L"placedToolTip.SizeChanged event raised.");
                if (placedToolTip->PlacementRect == nullptr)
                {
                    LOG_OUTPUT(L"Set placedToolTip.PlacementRect.");
                    placedToolTip->PlacementRect = wf::Rect(0, -50, 100, 150);
                }

                placedToolTipSizeChangedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(initialFocusButton, FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Give focus to the regular tool tip button to open its tool tip.");
        TestServices::KeyboardHelper->Tab();
        regularToolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect regularToolTipBounds = GetToolTipBounds(regularToolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Regular tool tip position: (%lf, %lf)", regularToolTipBounds.X, regularToolTipBounds.Y);
        });

        LOG_OUTPUT(L"Give focus to the placed tool tip button to open its tool tip.");
        TestServices::KeyboardHelper->Tab();
        regularToolTipClosedEvent->WaitForDefault();
        placedToolTipOpenedEvent->WaitForDefault();
        placedToolTipSizeChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect placedToolTipBounds = GetToolTipBounds(placedToolTip);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Placed tool tip position: (%lf, %lf)", placedToolTipBounds.X, placedToolTipBounds.Y);
            VERIFY_ARE_EQUAL(regularToolTipBounds.Y - 50, placedToolTipBounds.Y);
        });

        LOG_OUTPUT(L"Give focus to the final button to close the tool tip.");
        TestServices::KeyboardHelper->Tab();
        placedToolTipClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToolTipIntegrationTests::CanOpenToolTipWithUIASetFocus()
    {
        TestCleanupWrapper cleanup;

        auto toolTip = CreateToolTip();

        xaml_controls::Button^ topButton;
        xaml_controls::Button^ bottomButton;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::StackPanel();

            topButton = ref new xaml_controls::Button();
            topButton->Content = L"Top Button";
            topButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;

            rootPanel->Children->Append(topButton);

            bottomButton = ref new xaml_controls::Button();
            bottomButton->Content = L"Button with ToolTip";
            bottomButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;

            rootPanel->Children->Append(bottomButton);

            xaml_controls::ToolTipService::SetToolTip(bottomButton, toolTip);

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        OpenToolTip(bottomButton, toolTip, InputMode::UIA);

        CloseToolTip(toolTip, InputMode::None);
    }

    void ToolTipIntegrationTests::OpenCloseOpen()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        const auto& wh = TestServices::WindowHelper;
        const auto& u = TestServices::Utilities;

        xaml_controls::ToolTip^ toolTip;
        xaml_controls::Button^ button;

        RunOnUIThread([&]()
        {
            xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
            rect->Width = 20;
            rect->Height = 20;
            rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);

            toolTip = ref new xaml_controls::ToolTip();
            toolTip->Content = rect;

            xaml_shapes::Rectangle^ rect2 = ref new xaml_shapes::Rectangle();
            rect2->Width = 50;
            rect2->Height = 50;
            rect2->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);

            xaml_controls::ToolTipService::SetToolTip(rect2, toolTip);

            auto root = ref new xaml_controls::Canvas();
            root->Width = 100;
            root->Height = 100;
            root->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
            root->Children->Append(rect2);
            wh->WindowContent = root;
        });
        wh->WaitForIdle();

        //
        // This bug involves some precise timing with events being raised by Xaml. Do everything in a single frame to
        // set up the necessary order of operations to repro it.
        //
        // This bug can repro in the wild if these things all happen before Xaml processes the Popup.Closed event that
        // gets queued as part of these operations.
        //
        RunOnUIThread([&]()
        {
            //
            // Current ToolTip state: !m_bIsPopupPositioned && !m_bCallPerformPlacementAtNextPopupOpen
            // Current VSM state: Closed
            // Currently queued events: ---
            //

            //
            // Go through ToolTip::OnIsOpenChanged with an open ToolTip. This calls CPopup::Open, which will eventually
            // cause a Popup.Opened event to come back to be handled by ToolTip::OnPopupOpened.
            //
            toolTip->IsOpen = true;
            // Current ToolTip state: !m_bIsPopupPositioned && !m_bCallPerformPlacementAtNextPopupOpen
            // Current VSM state: Closed
            // Currently queued events: [Popup.Opened]

            //
            // Call UpdateLayout to cause ToolTip::OnToolTipSizeChanged. This is a synchronous call from layout to ToolTip.
            // OnToolTipSizeChanged calls ToolTip::PerformPlacement, which consumes !m_bIsPopupPositioned to call
            // GoToState("Closed") followed by GoToState("Opened"). PerformPlacement then marks m_bIsPopupPositioned.
            //
            toolTip->UpdateLayout();
            // Current ToolTip state: [m_bIsPopupPositioned] && !m_bCallPerformPlacementAtNextPopupOpen
            // Current VSM state: [Opened]
            // Currently queued events: Popup.Opened

            //
            // Go through ToolTip::OnIsOpenChanged again with a closed ToolTip. This calls UpdateLayoutState which calls
            // GoToState("Closed"). It also calls CPopup::Close, which will eventually cause a Popup.Closed event to come
            // back to be handled by ToolTip::OnPopupClosed.
            //
            toolTip->IsOpen = false;
            // Current ToolTip state: m_bIsPopupPositioned && !m_bCallPerformPlacementAtNextPopupOpen
            // Current VSM state: [Closed]
            // Currently queued events: Popup.Opened, [Popup.Closed]

            //
            // Go through ToolTip::OnIsOpenChanged again with an open ToolTip. This calls CPopup::Open, which will eventually
            // cause a Popup.Opened event to come back to be handled by ToolTip::OnPopupOpened. This ToolTip now has a m_wrPopup
            // since this is the second time it's been opened, so it also marks m_bCallPerformPlacementAtNextPopupOpen
            // to leave some work for the _next_ OnPopupOpened handler.
            //
            // Note that the next OnPopupOpened handler doesn't correspond to this CPopup::Open, but to the very first call
            // that queued an event yet to be raised.
            //
            toolTip->IsOpen = true;
            // Current ToolTip state: m_bIsPopupPositioned && [m_bCallPerformPlacementAtNextPopupOpen]
            // Current VSM state: Closed
            // Currently queued events: Popup.Opened, Popup.Closed, [Popup.Opened]

            //
            // Go through layout one more time to make it clean. This goes through ToolTip::OnToolTipSizeChanged to call
            // PerformPlacement, which no-ops because m_bIsPopupPositioned is set. Updating layout here prevents the
            // OnToolTipSizeChanged call from happening later.
            //
            toolTip->UpdateLayout();
            // Current ToolTip state: m_bIsPopupPositioned && m_bCallPerformPlacementAtNextPopupOpen
            // Current VSM state: Closed
            // Currently queued events: Popup.Opened, Popup.Closed, Popup.Opened
        });
        wh->WaitForIdle();

        //
        // After this point, the queued popup events get raised and handled by ToolTip.
        //

        //
        // The first Popup.Opened is handled by ToolTip::OnPopupOpened, which calls PerformPlacement because
        // m_bCallPerformPlacementAtNextPopupOpen is set. PerformPlacement does not kick off VSM state transitions
        // because m_bIsPopupPositioned is set. OnPopupOpened then clears m_bCallPerformPlacementAtNextPopupOpen.
        //
        // Current ToolTip state: m_bIsPopupPositioned && [!m_bCallPerformPlacementAtNextPopupOpen]
        // Current VSM state: Closed
        // Currently queued events: [Popup.Closed, Popup.Opened]
        //

        //
        // The Popup.Closed event is handled by ToolTip::OnPopupClosed, which clears m_bIsPopupPositioned.
        //
        // Current ToolTip state: [!m_bIsPopupPositioned] && !m_bCallPerformPlacementAtNextPopupOpen
        // Current VSM state: Closed
        // Currently queued events: [Popup.Opened]
        //

        //
        // The second Popup.Opened is handled by ToolTip::OnPopupOpened, which does not call PerformPlacement
        // because m_bCallPerformPlacementAtNextPopupOpen is cleared.
        //
        // Current ToolTip state: !m_bIsPopupPositioned && !m_bCallPerformPlacementAtNextPopupOpen
        // Current VSM state: Closed
        // Currently queued events: [---]
        //

        //
        // The app has now entered steady state. The ToolTip is left in the "Closed" state, even though it's open.
        //
        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Open");  // TODO: We only care that the ToolTip is drawing stuff, and not the rest of the tree
    }

    wf::Rect ToolTipIntegrationTests::GetToolTipBounds(xaml_controls::ToolTip^ toolTip)
    {
        wf::Rect toolTipBounds = {};
        HWND toolTipHwnd = reinterpret_cast<HWND>(TestServices::WindowHelper->ToolTip_GetWindow(toolTip));

        if (toolTipHwnd)
        {
            RunOnUIThread([&]()
            {
                RECT toolTipHwndBounds = {};
                VERIFY_IS_TRUE(GetWindowRect(toolTipHwnd, &toolTipHwndBounds));

                toolTipBounds.X = static_cast<float>(toolTipHwndBounds.left);
                toolTipBounds.Y = static_cast<float>(toolTipHwndBounds.top);
                toolTipBounds.Width = static_cast<float>(toolTipHwndBounds.right - toolTipHwndBounds.left);
                toolTipBounds.Height = static_cast<float>(toolTipHwndBounds.bottom - toolTipHwndBounds.top);
            });
        }
        else
        {
            toolTipBounds = ControlHelper::GetBounds(toolTip);
        }

        return toolTipBounds;
    }

    void ToolTipIntegrationTests::ValidateOnlyCtrlDimissToolTip()
    {
        TestCleanupWrapper cleanup;

        wf::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);
        xaml_controls::Button^ button = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Margin='0,25,0,0' "
                L"            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"             <Button Content='Button no tooltip'/>"
                L"             <Button x:Name='bt' Width='200' Content='Button with a simple ToolTip.'>"
                L"              <ToolTipService.ToolTip>"
                L"                  <ToolTip x:Name='toolTip'>"
                L"                      Simple ToolTip"
                L"                  </ToolTip>"
                L"              </ToolTipService.ToolTip>"
                L"             </Button>"
                L"</StackPanel>"));

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"bt"));
            VERIFY_IS_NOT_NULL(button);
            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::ToolTip^ toolTip = nullptr;

        auto toolTipOpenedEvent = std::make_shared<Event>();
        auto toolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTipClosedEvent = std::make_shared<Event>();
        auto toolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            toolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"toolTip"));

            toolTipOpenedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([toolTipOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: ToolTip Opened event fired!");
                toolTipOpenedEvent->Set();
            }));
            toolTipClosedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([toolTipClosedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: ToolTip Closed event fired!");
                toolTipClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        // Move mouse to text block and check tooltip
        LOG_OUTPUT(L"OpenToolTip: Focusing button");
        RunOnUIThread([&]()
        {
            button->Focus(FocusState::Keyboard);
        });

        toolTipOpenedEvent->WaitForDefault();
        Sleep(1000);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
        });

        // Ctrl + o
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_o#$u$_o#$u$_ctrlscan");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
        });

        // Shift + r
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_r#$u$_r#$u$_shift");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
        });

        // Ctrl + c
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_c#$u$_c#$u$_ctrlscan");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
        });

        // Ctrl only
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$u$_ctrlscan");
        TestServices::WindowHelper->WaitForIdle();

        toolTipClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toolTip->IsOpen);
        });
    }

    void ToolTipIntegrationTests::ValidatePointerOverToolTip()
    {
        TestCleanupWrapper cleanup;

        wf::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);
        xaml_controls::Button^ button = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        
        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Margin='0,25,0,0' "
                L"            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"             <Button Content='Button no tooltip'/>"
                L"             <Button x:Name='bt' Width='200' Content='Button with a simple ToolTip.'>"
                L"              <ToolTipService.ToolTip>"
                L"                  <ToolTip x:Name='toolTip'>"
                L"                      Simple ToolTip"
                L"                  </ToolTip>"
                L"              </ToolTipService.ToolTip>"
                L"             </Button>"
                L"</StackPanel>"));

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"bt"));
            VERIFY_IS_NOT_NULL(button);
            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::ToolTip^ toolTip = nullptr;

        auto toolTipOpenedEvent = std::make_shared<Event>();
        auto toolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTipClosedEvent = std::make_shared<Event>();
        auto toolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            toolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"toolTip"));

            toolTipOpenedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([toolTipOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: ToolTip Opened event fired!");
                toolTipOpenedEvent->Set();
            }));
            toolTipClosedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([toolTipClosedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: ToolTip Closed event fired!");
                toolTipClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->MoveMouse(wf::Point(1,1));
        TestServices::WindowHelper->WaitForIdle();

        // Move mouse to text block and check tooltip
        TestServices::InputHelper->MoveMouse(button);
        TestServices::WindowHelper->WaitForIdle();

        toolTipOpenedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
        });

        Sleep(1000);

        // move to tooltip
        TestServices::InputHelper->MoveMouse(toolTip);
        TestServices::WindowHelper->WaitForIdle();

        // There is timer to cancel the ToolTip. Wait for a safezone check
        Sleep(2000);
        TestServices::WindowHelper->WaitForIdle();

        // Verify Popup is still open after pointer over tooltip
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
        });

        // Close the popup after mouse is out of safe zone
        TestServices::InputHelper->MoveMouse(wf::Point(300,300));
        TestServices::WindowHelper->WaitForIdle();

        toolTipClosedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toolTip->IsOpen);
        });
    }

    void ToolTipIntegrationTests::VerifyControlPositionChangeDismissToolTip()
    {
        TestCleanupWrapper cleanup;

        wf::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);
        xaml_controls::Button^ button = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        
        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Margin='0,25,0,0' "
                L"            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"             <Button Content='Button no tooltip'/>"
                L"             <Button x:Name='bt' Width='200' Content='Button with a simple ToolTip.'>"
                L"              <ToolTipService.ToolTip>"
                L"                  <ToolTip x:Name='toolTip'>"
                L"                      Simple ToolTip"
                L"                  </ToolTip>"
                L"              </ToolTipService.ToolTip>"
                L"             </Button>"
                L"</StackPanel>"));

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"bt"));
            VERIFY_IS_NOT_NULL(button);
            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::ToolTip^ toolTip = nullptr;

        auto toolTipOpenedEvent = std::make_shared<Event>();
        auto toolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTipClosedEvent = std::make_shared<Event>();
        auto toolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            toolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"toolTip"));

            toolTipOpenedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([toolTipOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: ToolTip Opened event fired!");
                toolTipOpenedEvent->Set();
            }));
            toolTipClosedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([toolTipClosedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: ToolTip Closed event fired!");
                toolTipClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->MoveMouse(wf::Point(1,1));
        TestServices::WindowHelper->WaitForIdle();

        // Move mouse to text block and check tooltip
        TestServices::InputHelper->MoveMouse(button);
        TestServices::WindowHelper->WaitForIdle();

        toolTipOpenedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);

            // change the timer
            button->Width += 10;
        });

        // There is timer to cancel the ToolTip. Wait for a safezone check
        Sleep(2000);
        TestServices::WindowHelper->WaitForIdle();

        // Verify Popup is still closed after tooltip owner size changed
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toolTip->IsOpen);
        });
    }

    void ToolTipIntegrationTests::VerifyPointerMoveInsideControlNotDismissToolTip()
    {
        TestCleanupWrapper cleanup;

        wf::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);
        xaml_controls::Button^ button = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        
        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Margin='0,25,0,0' "
                L"            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"             <Button Content='Button no tooltip'/>"
                L"             <Button x:Name='bt' Width='200' Height='200' Content='Button with a simple ToolTip.'>"
                L"              <ToolTipService.ToolTip>"
                L"                  <ToolTip x:Name='toolTip'>"
                L"                      Simple ToolTip"
                L"                  </ToolTip>"
                L"              </ToolTipService.ToolTip>"
                L"             </Button>"
                L"</StackPanel>"));

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"bt"));
            VERIFY_IS_NOT_NULL(button);
            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::ToolTip^ toolTip = nullptr;

        auto toolTipOpenedEvent = std::make_shared<Event>();
        auto toolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTipClosedEvent = std::make_shared<Event>();
        auto toolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            toolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPanel->FindName(L"toolTip"));

            toolTipOpenedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([toolTipOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: ToolTip Opened event fired!");
                toolTipOpenedEvent->Set();
            }));
            toolTipClosedRegistration.Attach(toolTip, ref new xaml::RoutedEventHandler([toolTipClosedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"OpenToolTip: ToolTip Closed event fired!");
                toolTipClosedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->MoveMouse(wf::Point(1,1));
        TestServices::WindowHelper->WaitForIdle();

        // Move mouse to text block and check tooltip
        TestServices::InputHelper->MoveMouse(button);
        TestServices::WindowHelper->WaitForIdle();

        toolTipOpenedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
        });

        Sleep(1000);

        wf::Point targetPoint = {};
        RunOnUIThread([&]()
        {
            auto buttonBounds = ControlHelper::GetBounds(button);
            targetPoint = wf::Point(buttonBounds.Left + 10, buttonBounds.Top + 10);
        });

        // move to tooltip in the button
        TestServices::InputHelper->MoveMouse(targetPoint);
        TestServices::WindowHelper->WaitForIdle();

        // There is timer to cancel the ToolTip. Wait for a safezone check
        Sleep(2000);
        TestServices::WindowHelper->WaitForIdle();

        // Verify Popup is still open after pointer over tooltip
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toolTip->IsOpen);
        });

        // Close the popup after mouse is out of safe zone
        TestServices::InputHelper->MoveMouse(wf::Point(300,300));
        TestServices::WindowHelper->WaitForIdle();

        toolTipClosedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toolTip->IsOpen);
        });
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ToolTip
