// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CommandBarIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <ControlHelper.h>
#include <array>
#include <FileLoader.h>
#include <CommonInputHelper.h>
#include <CustomTypeMetadataProvider.h>
#include <CustomAppBarButton.h>
#include <WUCRenderingScopeGuard.h>
#include <FlyoutHelper.h>
#include <ChangeDpi.h>
#include <Utils.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

using Colors = Microsoft::UI::Colors;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace CommandBar {

    bool CommandBarIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        featureDisableTransitionsForTest.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTransitionsForTest, true);
        return true;
    }

    bool CommandBarIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
        return true;
    }

    bool CommandBarIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void CommandBarIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::CommandBar>::CanInstantiate();
    }

    void CommandBarIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        auto hasLoadedEvent = std::make_shared<Event>();
        auto hasUnloadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);
        auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Unloaded);

        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();

            cmdBar = ref new xaml_controls::CommandBar();
            loadedRegistration.Attach(cmdBar, [&]() { hasLoadedEvent->Set(); });
            unloadedRegistration.Attach(cmdBar, [&]() { hasUnloadedEvent->Set(); });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = true;
            page->BottomAppBar = cmdBar;
        });
        hasLoadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });
        hasUnloadedEvent->WaitForDefault();
    }

    void CommandBarIntegrationTests::CanReapplyTemplate()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            loadedRegistration.Attach(cmdBar, [&]() { loadedEvent->Set(); });

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = cmdBar;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto cmdBarTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                LR"(<ControlTemplate
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        TargetType='CommandBar'>
                        <Grid x:Name='LayoutRoot'>
                            <Grid.Clip>
                                <RectangleGeometry Rect='{Binding RelativeSource={RelativeSource TemplatedParent}, Path=TemplateSettings.ClipRect}'>
                                    <RectangleGeometry.Transform>
                                        <TranslateTransform x:Name='ClipGeometryTransform' Y='{Binding RelativeSource={RelativeSource TemplatedParent}, Path=TemplateSettings.CompactVerticalDelta}'/>
                                    </RectangleGeometry.Transform>
                                </RectangleGeometry>
                            </Grid.Clip>
                            <Grid x:Name='ContentRoot'>
                                <Grid.ColumnDefinitions>
                                    <ColumnDefinition Width='*'/>
                                    <ColumnDefinition Width='Auto'/>
                                </Grid.ColumnDefinitions>
                                <Grid.RenderTransform>
                                    <TranslateTransform x:Name='ContentTransform'/>
                                </Grid.RenderTransform>
                                <Grid>
                                    <Grid.ColumnDefinitions>
                                        <ColumnDefinition Width='*'/>
                                        <ColumnDefinition Width='Auto'/>
                                    </Grid.ColumnDefinitions>
                                    <ContentControl x:Name='ContentControl'
                                        Content='{TemplateBinding Content}'
                                        ContentTemplate='{TemplateBinding ContentTemplate}'/>
                                    <ItemsControl x:Name='PrimaryItemsControl'>
                                        <ItemsControl.ItemsPanel>
                                            <ItemsPanelTemplate>
                                                <StackPanel Orientation='Horizontal' />
                                            </ItemsPanelTemplate>
                                        </ItemsControl.ItemsPanel>
                                    </ItemsControl>
                                </Grid>
                                <Button x:Name='MoreButton' />
                                <Popup x:Name='OverflowPopup'>
                                    <Popup.RenderTransform>
                                        <TranslateTransform x:Name='OverflowPopupOffsetTransform'/>
                                    </Popup.RenderTransform>
                                    <Grid x:Name='OverflowContentRoot'>
                                        <Grid.Clip>
                                            <RectangleGeometry x:Name='OverflowContentRootClip'/>
                                        </Grid.Clip>
                                        <Grid.RenderTransform>
                                            <TranslateTransform x:Name='OverflowContentRootTransform'
                                                X='{Binding RelativeSource={RelativeSource TemplatedParent}, Path=CommandBarTemplateSettings.OverflowContentHorizontalOffset}'/>
                                        </Grid.RenderTransform>
                                        <CommandBarOverflowPresenter x:Name='SecondaryItemsControl'
                                            Style='{TemplateBinding CommandBarOverflowPresenterStyle}'
                                            IsEnabled='False'
                                            IsTabStop='False'>
                                            <CommandBarOverflowPresenter.RenderTransform>
                                                <TranslateTransform x:Name='OverflowContentTransform'/>
                                            </CommandBarOverflowPresenter.RenderTransform>
                                            <CommandBarOverflowPresenter.ItemContainerStyle>
                                                <Style TargetType='FrameworkElement'>
                                                    <Setter Property='HorizontalAlignment' Value='Stretch'/>
                                                    <Setter Property='Width' Value='NaN'/>
                                                </Style>
                                            </CommandBarOverflowPresenter.ItemContainerStyle>
                                        </CommandBarOverflowPresenter>
                                    </Grid>
                                </Popup>
                                <Rectangle x:Name='HighContrastBorder' x:DeferLoadStrategy='Lazy' Grid.ColumnSpan='2'  Visibility='Collapsed' VerticalAlignment='Stretch' Stroke='{ThemeResource SystemControlForegroundTransparentBrush}' StrokeThickness='1'/>
                            </Grid>
                        </Grid>
                    </ControlTemplate>)"));

            cmdBar->Template = cmdBarTemplate;
            cmdBar->ApplyTemplate();
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::CanOpenAndCloseUsingAPI()
    {
        TestCleanupWrapper cleanup;

        auto openFunc = [] (xaml_controls::CommandBar^ cmdBar) { RunOnUIThread([cmdBar](){ cmdBar->IsOpen = true; }); };
        auto closeFunc = [] (xaml_controls::CommandBar^ cmdBar) { RunOnUIThread([cmdBar](){ cmdBar->IsOpen = false; }); };

        ValidateOpenAndCloseWorker(openFunc, closeFunc);
    }

    void CommandBarIntegrationTests::CanOpenAndCloseUsingMoreButtonProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanOpenAndCloseUsingMoreButton();
    }

    void CommandBarIntegrationTests::CanOpenAndCloseUsingMoreButtonDropShadow()
    {
        CanOpenAndCloseUsingMoreButton();
    }

    void CommandBarIntegrationTests::CanOpenAndCloseUsingMoreButton()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        auto openAndCloseFunc = [&] (xaml_controls::CommandBar^ cmdBar)
        {
            auto moreButton = GetMoreButton(cmdBar);
            TestServices::InputHelper->Tap(moreButton);
        };

        ValidateOpenAndCloseWorker(openAndCloseFunc, openAndCloseFunc, true);
    }

    void CommandBarIntegrationTests::DoesCloseOnPrimaryCommandSelection()
    {
        TestCleanupWrapper cleanup;

        auto openFunc = [] (xaml_controls::CommandBar^ cmdBar) { RunOnUIThread([cmdBar](){ cmdBar->IsOpen = true; }); };
        auto closeFunc = [] (xaml_controls::CommandBar^ cmdBar)
        {
            xaml::FrameworkElement^ tapTarget = nullptr;

            RunOnUIThread([&] ()
            {
                tapTarget = safe_cast<xaml::FrameworkElement^>(cmdBar->PrimaryCommands->GetAt(0));
            });

            TestServices::InputHelper->Tap(tapTarget);
        };

        ValidateOpenAndCloseWorker(openFunc, closeFunc);
    }

    void CommandBarIntegrationTests::DoesCloseOnSecondaryCommandSelection()
    {
        TestCleanupWrapper cleanup;

        auto openFunc = [](xaml_controls::CommandBar^ cmdBar) { RunOnUIThread([cmdBar](){ cmdBar->IsOpen = true; }); };
        auto closeFunc = [](xaml_controls::CommandBar^ cmdBar)
        {
            xaml::FrameworkElement^ tapTarget = nullptr;

            RunOnUIThread([&]()
            {
                tapTarget = safe_cast<xaml::FrameworkElement^>(cmdBar->SecondaryCommands->GetAt(0));
            });

            TestServices::InputHelper->Tap(tapTarget);
        };

        ValidateOpenAndCloseWorker(openFunc, closeFunc);
    }

    void CommandBarIntegrationTests::ValidateOpenAndCloseWorker(std::function<void(xaml_controls::CommandBar^)> openFunc, std::function<void(xaml_controls::CommandBar^)> closeFunc, bool validateDCompTree)
    {
        xaml_controls::CommandBar^ cmdBar = nullptr;

        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Center; // Center it to get it out from under the status bar.

            // Add at least one primary command and one secondary command to the CommandBar.
            cmdBar->PrimaryCommands->Append(ref new xaml_controls::AppBarButton());
            cmdBar->SecondaryCommands->Append(ref new xaml_controls::AppBarButton());

            openedRegistration.Attach(cmdBar, [&]() { openedEvent->Set(); });
            closedRegistration.Attach(cmdBar, [&]() { closedEvent->Set(); });

            auto root = ref new xaml_controls::Grid();
            root->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::White);
            root->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        openFunc(cmdBar);
        TestServices::WindowHelper->WaitForIdle();
        openedEvent->WaitForDefault();

        if (validateDCompTree)
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        closeFunc(cmdBar);
        TestServices::WindowHelper->WaitForIdle();
        closedEvent->WaitForDefault();
    }

    void CommandBarIntegrationTests::CanAddToAndRemoveFromCommandCollections()
    {
        TestCleanupWrapper cleanup;

        // Make sure we can add/remove items to/from our command collections.
        RunOnUIThread([&]()
        {
            xaml_controls::CommandBar^ cmdBar = ref new xaml_controls::CommandBar();

            VERIFY_IS_TRUE(cmdBar->PrimaryCommands->Size == 0);
            VERIFY_IS_TRUE(cmdBar->SecondaryCommands->Size == 0);

            auto btn1 = ref new xaml_controls::AppBarButton();
            cmdBar->PrimaryCommands->Append(btn1);
            VERIFY_IS_TRUE(btn1 == dynamic_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(0)));

            auto btn2 = ref new xaml_controls::AppBarToggleButton();
            cmdBar->PrimaryCommands->Append(btn2);
            VERIFY_IS_TRUE(btn2 == cmdBar->PrimaryCommands->GetAt(1));

            cmdBar->PrimaryCommands->RemoveAt(1);
            VERIFY_IS_TRUE(cmdBar->PrimaryCommands->Size == 1);

            cmdBar->PrimaryCommands->RemoveAt(0);
            VERIFY_IS_TRUE(cmdBar->PrimaryCommands->Size == 0);

            auto btn3 = ref new xaml_controls::AppBarButton();
            cmdBar->SecondaryCommands->Append(btn3);
            VERIFY_IS_TRUE(btn3 == cmdBar->SecondaryCommands->GetAt(0));

            auto btn4 = ref new xaml_controls::AppBarToggleButton();
            cmdBar->SecondaryCommands->Append(btn4);
            VERIFY_IS_TRUE(btn4 == cmdBar->SecondaryCommands->GetAt(1));

            cmdBar->SecondaryCommands->RemoveAt(1);
            VERIFY_IS_TRUE(cmdBar->SecondaryCommands->Size == 1);

            cmdBar->SecondaryCommands->RemoveAt(0);
            VERIFY_IS_TRUE(cmdBar->SecondaryCommands->Size == 0);

            cmdBar->SecondaryCommands->Append(btn3);
            VERIFY_IS_TRUE(btn3 == cmdBar->SecondaryCommands->GetAt(0));

            cmdBar->SecondaryCommands->SetAt(0, btn4);
            VERIFY_IS_TRUE(btn4 == cmdBar->SecondaryCommands->GetAt(0));

            cmdBar->SecondaryCommands->RemoveAtEnd();
            VERIFY_IS_TRUE(cmdBar->SecondaryCommands->Size == 0);

            cmdBar->PrimaryCommands->Append(btn1);
            VERIFY_IS_TRUE(btn1 == cmdBar->PrimaryCommands->GetAt(0));

            cmdBar->PrimaryCommands->Append(btn2);
            VERIFY_IS_TRUE(btn2 == cmdBar->PrimaryCommands->GetAt(1));

            cmdBar->SecondaryCommands->Append(btn3);
            VERIFY_IS_TRUE(btn3 == cmdBar->SecondaryCommands->GetAt(0));

            cmdBar->SecondaryCommands->Append(btn4);
            VERIFY_IS_TRUE(btn4 == cmdBar->SecondaryCommands->GetAt(1));

            cmdBar->PrimaryCommands->Clear();
            VERIFY_IS_TRUE(cmdBar->PrimaryCommands->Size == 0);

            cmdBar->SecondaryCommands->Clear();
            VERIFY_IS_TRUE(cmdBar->SecondaryCommands->Size == 0);
        });

        // Make sure we can add items to our command collections via the parser.
        RunOnUIThread([&]()
        {
            xaml_controls::CommandBar^ cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                        <AppBarToggleButton Label="btn1"/>
                        <AppBarButton Label="btn2"/>
                        <CommandBar.SecondaryCommands>
                            <AppBarToggleButton Label="btn3"/>
                            <AppBarButton Label="btn4"/>
                        </CommandBar.SecondaryCommands>
                    </CommandBar>)"));

            VERIFY_IS_NOT_NULL(cmdBar);
            VERIFY_IS_TRUE(cmdBar->PrimaryCommands->Size == 2);
            VERIFY_IS_TRUE(cmdBar->SecondaryCommands->Size == 2);

            auto btn1 = dynamic_cast<xaml_controls::AppBarToggleButton^>(cmdBar->PrimaryCommands->GetAt(0));
            VERIFY_IS_NOT_NULL(btn1);
            VERIFY_IS_TRUE(btn1->Label == "btn1");

            auto btn2 = dynamic_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(1));
            VERIFY_IS_NOT_NULL(btn2);
            VERIFY_IS_TRUE(btn2->Label == "btn2");

            auto btn3 = dynamic_cast<xaml_controls::AppBarToggleButton^>(cmdBar->SecondaryCommands->GetAt(0));
            VERIFY_IS_NOT_NULL(btn3);
            VERIFY_IS_TRUE(btn3->Label == "btn3");

            auto btn4 = dynamic_cast<xaml_controls::AppBarButton^>(cmdBar->SecondaryCommands->GetAt(1));
            VERIFY_IS_NOT_NULL(btn4);
            VERIFY_IS_TRUE(btn4->Label == "btn4");
        });
    }

    void CommandBarIntegrationTests::ValidateOverflowPlacement()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"ValidateOverflowPosition: Opened Up, Aligned Right, FlowDirection=LTR");
        ValidateOverflowPlacementWorker(OverflowOpenDirection::Up, OverflowAlignment::Right, false /*isRTL*/);

        LOG_OUTPUT(L"ValidateOverflowPosition: Opened Up, Aligned Left, FlowDirection=LTR");
        ValidateOverflowPlacementWorker(OverflowOpenDirection::Up, OverflowAlignment::Left, false /*isRTL*/);

        LOG_OUTPUT(L"ValidateOverflowPosition: Opened Down, Aligned Right, FlowDirection=LTR");
        ValidateOverflowPlacementWorker(OverflowOpenDirection::Down, OverflowAlignment::Right, false /*isRTL*/);

        LOG_OUTPUT(L"ValidateOverflowPosition: Opened Down, Aligned Left, FlowDirection=LTR");
        ValidateOverflowPlacementWorker(OverflowOpenDirection::Down, OverflowAlignment::Left, false /*isRTL*/);

        // Validate the same scenarios, except with FlowDirection=RTL
        LOG_OUTPUT(L"ValidateOverflowPosition: Opened Up, Aligned Right, FlowDirection=RTL");
        ValidateOverflowPlacementWorker(OverflowOpenDirection::Up, OverflowAlignment::Right, true /*isRTL*/);

        LOG_OUTPUT(L"ValidateOverflowPosition: Opened Up, Aligned Left, FlowDirection=RTL");
        ValidateOverflowPlacementWorker(OverflowOpenDirection::Up, OverflowAlignment::Left, true /*isRTL*/);

        LOG_OUTPUT(L"ValidateOverflowPosition: Opened Down, Aligned Right, FlowDirection=RTL");
        ValidateOverflowPlacementWorker(OverflowOpenDirection::Down, OverflowAlignment::Right, true /*isRTL*/);

        LOG_OUTPUT(L"ValidateOverflowPosition: Opened Down, Aligned Left, FlowDirection=RTL");
        ValidateOverflowPlacementWorker(OverflowOpenDirection::Down, OverflowAlignment::Left, true /*isRTL*/);
    }

    void CommandBarIntegrationTests::ValidateOverflowPlacementWorker(OverflowOpenDirection openDirection, OverflowAlignment alignment, bool isRTL)
    {
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->Width = 150;
            cmdBar->IsOpen = true;

            auto menuitem = ref new xaml_controls::AppBarButton();
            menuitem->Label = L"menu item";
            menuitem->Width = 200;
            cmdBar->SecondaryCommands->Append(menuitem);

            cmdBar->VerticalAlignment = (openDirection == OverflowOpenDirection::Up ? xaml::VerticalAlignment::Bottom : xaml::VerticalAlignment::Top);
            cmdBar->HorizontalAlignment = (alignment == OverflowAlignment::Left ? xaml::HorizontalAlignment::Left : xaml::HorizontalAlignment::Right);

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(cmdBar);
            if (isRTL)
            {
                root->FlowDirection = xaml::FlowDirection::RightToLeft;
            }

            loadedRegistration.Attach(root, [&]()
            {
                LOG_OUTPUT(L"Grid.Loaded raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto overflowContentRoot = safe_cast<xaml::UIElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar));
            VERIFY_IS_NOT_NULL(overflowContentRoot);

            auto transform = overflowContentRoot->TransformToVisual(cmdBar);
            auto overflowTransformedBounds = transform->TransformBounds(wf::Rect(0, 0, overflowContentRoot->DesiredSize.Width, overflowContentRoot->DesiredSize.Height));

            if (openDirection == OverflowOpenDirection::Up)
            {
                VERIFY_IS_LESS_THAN(overflowTransformedBounds.Y, 0);
            }
            else // OverflowOpenDirection::Down
            {
                VERIFY_IS_GREATER_THAN(overflowTransformedBounds.Y, 0);
            }

            if (alignment == OverflowAlignment::Left)
            {
                VERIFY_ARE_EQUAL(overflowTransformedBounds.X, 0);
            }
            else // OverflowAlignment::Right
            {
                VERIFY_ARE_EQUAL(overflowTransformedBounds.X + overflowTransformedBounds.Width, cmdBar->ActualWidth);
            }
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::ValidateOverflowSnapsToWindowWidth()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);

        double expectedWidth = 400;

        // Override the window size to be < 480 to simulate the conditions
        // under which the overflow menu will snap.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(static_cast<float>(expectedWidth), 600));

        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->IsOpen = true;

            auto button = ref new xaml_controls::AppBarButton();
            button->Label = "menu item";

            cmdBar->SecondaryCommands->Append(button);

            loadedRegistration.Attach(cmdBar, [&]()
            {
                LOG_OUTPUT(L"CommandBar.Loaded raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = cmdBar;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto overflowContentRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar));
            VERIFY_IS_NOT_NULL(overflowContentRoot);

            VERIFY_ARE_EQUAL(overflowContentRoot->MinWidth, expectedWidth);
            VERIFY_ARE_EQUAL(overflowContentRoot->ActualWidth, expectedWidth);
        });
    }

    void CommandBarIntegrationTests::ValidateOverflowMaxHeight()
    {
        TestCleanupWrapper cleanup;

        const double overflowHeight = 300;
        // 40 for WindowedPopupPadding
        double expectedHeight = overflowHeight + 40;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, static_cast<float>(overflowHeight * 2)));

        // We add a rectangle to give us extra space in which to do translate transforms
        // when using windowed popups, so we add that to the max height and need to account for it.
        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            expectedHeight += 64;
        }

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);

        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->IsOpen = true;

            for (size_t i = 0; i < 50; ++i)
            {
                auto button = ref new xaml_controls::AppBarButton();
                button->Label = "menu item";

                cmdBar->SecondaryCommands->Append(button);
            }

            loadedRegistration.Attach(cmdBar, [&]()
            {
                LOG_OUTPUT(L"CommandBar.Loaded raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = cmdBar;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto overflowContentRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar));
            VERIFY_IS_NOT_NULL(overflowContentRoot);

            VERIFY_ARE_EQUAL(overflowContentRoot->MaxHeight, expectedHeight);
            VERIFY_ARE_EQUAL(overflowContentRoot->ActualHeight, expectedHeight);
        });
    }

    void CommandBarIntegrationTests::CanResizeCommandBarAfterOpeningAndClosing()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Button^ moreButton = nullptr;
        wf::Point originalMoreButtonOffset = {};

        RunOnUIThread([&]()
        {
            cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" Margin="0,0,100,0">
                        <AppBarButton Label="btn1"/>
                        <AppBarButton Label="btn2"/>
                        <AppBarButton Label="btn3"/>
                        <AppBarButton Label="btn4"/>
                        <AppBarButton Label="btn5"/>
                        <CommandBar.SecondaryCommands>
                            <AppBarButton Label="btn1"/>
                            <AppBarButton Label="btn2"/>
                            <AppBarButton Label="btn3"/>
                        </CommandBar.SecondaryCommands>
                    </CommandBar>)"));

            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton"));
            cmdBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            originalMoreButtonOffset = moreButton->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            cmdBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->Margin = xaml::ThicknessHelper::FromLengths(0, 0, 0, 0);
            cmdBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto newMoreButtonPosition = moreButton->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            VERIFY_IS_GREATER_THAN(newMoreButtonPosition.X, originalMoreButtonOffset.X);

            cmdBar->IsOpen = false;
        });
    }

    void CommandBarIntegrationTests::CanUseLargeAppBarButton()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_controls::Page^ page = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ appBarButton = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                      <CommandBar.SecondaryCommands>
                        <AppBarButton Label="AppBarButton" Height="1000">
                          <AppBarButton.Flyout>
                            <MenuFlyout>
                              <MenuFlyoutItem>MenuFlyoutItem</MenuFlyoutItem>
                            </MenuFlyout>
                          </AppBarButton.Flyout>
                        </AppBarButton>
                      </CommandBar.SecondaryCommands>
                    </CommandBar>)"));
            VERIFY_IS_NOT_NULL(cmdBar);

            appBarButton = safe_cast<xaml_controls::AppBarButton^>(cmdBar->SecondaryCommands->GetAt(0));
            VERIFY_IS_NOT_NULL(appBarButton);

            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();
            VERIFY_IS_NOT_NULL(page);

            page->BottomAppBar = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tabbing into the BottomAppBar's SecondaryCommands.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Opening the BottomAppBar's SecondaryCommands.");
        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tabbing into the tall AppBarButton.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Scrolling down within the AppBarButton.");
        TestServices::KeyboardHelper->PageDown();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over the AppBarButton.");
        TestServices::InputHelper->MoveMouse(appBarButton);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Holding the AppBarButton.");
        TestServices::InputHelper->Hold(appBarButton);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Closing the BottomAppBar.");
            cmdBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::ValidateMoreButtonVisualInDisabledState()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::FontIcon^ ellipsisIcon = nullptr;
        xaml_media::Brush^ expectedBrushEnabled = nullptr;
        xaml_media::Brush^ expectedBrushDisabled = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            TestServices::WindowHelper->WindowContent = cmdBar;

            expectedBrushEnabled = safe_cast<xaml_media::Brush^>(xaml::Application::Current->Resources->Lookup(L"TextFillColorPrimaryBrush"));
            expectedBrushDisabled = safe_cast<xaml_media::Brush^>(xaml::Application::Current->Resources->Lookup(L"TextFillColorDisabledBrush"));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the ellipsis is the correct color in the Enabled CommandBar:
        RunOnUIThread([&]()
        {
            ellipsisIcon = safe_cast<xaml_controls::FontIcon^>(TreeHelper::GetVisualChildByName(cmdBar, L"EllipsisIcon"));

            VERIFY_ARE_EQUAL(expectedBrushEnabled, ellipsisIcon->Foreground);

            cmdBar->IsEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the ellipsis is the correct color in the Disabled CommandBar:
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedBrushDisabled, ellipsisIcon->Foreground);
        });
    }

    void CommandBarIntegrationTests::ValidateAppBarButtonsHaveInvisibleLabelsWhenClosed()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ button = nullptr;
        xaml_controls::TextBlock^ buttonLabel = nullptr;
        xaml_controls::AppBarToggleButton^ toggleButton = nullptr;
        xaml_controls::TextBlock^ toggleButtonLabel = nullptr;
        xaml_controls::AppBarButton^ buttonSecondary = nullptr;
        xaml_controls::TextBlock^ buttonSecondaryLabel = nullptr;
        xaml_controls::AppBarToggleButton^ toggleButtonSecondary = nullptr;
        xaml_controls::TextBlock^ toggleButtonSecondaryLabel = nullptr;

        // Setup our environment.
        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();

            button = ref new xaml_controls::AppBarButton();
            button->Label = "First button";
            cmdBar->PrimaryCommands->Append(button);

            toggleButton = ref new xaml_controls::AppBarToggleButton();
            toggleButton->Label = "Second button";
            cmdBar->PrimaryCommands->Append(toggleButton);

            buttonSecondary = ref new xaml_controls::AppBarButton();
            buttonSecondary->Label = "First button";
            cmdBar->SecondaryCommands->Append(buttonSecondary);

            toggleButtonSecondary = ref new xaml_controls::AppBarToggleButton();
            toggleButtonSecondary->Label = "Second button";
            cmdBar->SecondaryCommands->Append(toggleButtonSecondary);

            TestServices::WindowHelper->WindowContent = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            buttonLabel = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(button, L"TextLabel"));
            toggleButtonLabel = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(toggleButton, L"TextLabel"));

            VERIFY_ARE_EQUAL(buttonLabel->Visibility, xaml::Visibility::Collapsed);
            VERIFY_ARE_EQUAL(toggleButtonLabel->Visibility, xaml::Visibility::Collapsed);

            cmdBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // We have to wait for the overflow popup to be open to query these.
            buttonSecondaryLabel = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(buttonSecondary, L"OverflowTextLabel"));
            toggleButtonSecondaryLabel = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(toggleButtonSecondary, L"OverflowTextLabel"));

            VERIFY_ARE_EQUAL(buttonLabel->Visibility, xaml::Visibility::Visible);
            VERIFY_ARE_EQUAL(toggleButtonLabel->Visibility, xaml::Visibility::Visible);
            VERIFY_ARE_EQUAL(buttonSecondaryLabel->Visibility, xaml::Visibility::Visible);
            VERIFY_ARE_EQUAL(toggleButtonSecondaryLabel->Visibility, xaml::Visibility::Visible);

            cmdBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(buttonLabel->Visibility, xaml::Visibility::Collapsed);
            VERIFY_ARE_EQUAL(toggleButtonLabel->Visibility, xaml::Visibility::Collapsed);

            // Secondary buttons' label visibilities are unaffected, since the buttons aren't touched when IsOpen is false.
            VERIFY_ARE_EQUAL(buttonSecondaryLabel->Visibility, xaml::Visibility::Visible);
            VERIFY_ARE_EQUAL(toggleButtonSecondaryLabel->Visibility, xaml::Visibility::Visible);
        });
    }

    void CommandBarIntegrationTests::ValidateAppBarButtonsAreOffsetWithAppBarToggleButtons()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ button = nullptr;
        xaml_controls::TextBlock^ buttonLabel = nullptr;
        xaml_controls::AppBarToggleButton^ toggleButton = nullptr;

        auto buttonLoadedEvent = std::make_shared<Event>();
        auto buttonUnloadedEvent = std::make_shared<Event>();
        auto toggleButtonLoadedEvent = std::make_shared<Event>();
        auto toggleButtonUnloadedEvent = std::make_shared<Event>();
        auto buttonLoadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, Loaded);
        auto buttonUnloadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, Unloaded);
        auto toggleButtonLoadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBarToggleButton, Loaded);
        auto toggleButtonUnloadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBarToggleButton, Unloaded);

        double originalAppBarButtonMargin = 0;

        // Setup our environment.
        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->IsOpen = true;

            button = ref new xaml_controls::AppBarButton();
            button->Label = "First button";
            cmdBar->SecondaryCommands->Append(button);

            toggleButton = ref new xaml_controls::AppBarToggleButton();
            toggleButton->Label = "Second button";

            buttonLoadedRegistration.Attach(button, [&]()
            {
                LOG_OUTPUT(L"AppBarButton loaded.");
                buttonLoadedEvent->Set();
            });

            buttonUnloadedRegistration.Attach(button, [&]()
            {
                LOG_OUTPUT(L"AppBarButton unloaded.");
                buttonUnloadedEvent->Set();
            });

            toggleButtonLoadedRegistration.Attach(toggleButton, [&]()
            {
                LOG_OUTPUT(L"AppBarToggleButton loaded.");
                toggleButtonLoadedEvent->Set();
            });

            toggleButtonUnloadedRegistration.Attach(toggleButton, [&]()
            {
                LOG_OUTPUT(L"AppBarToggleButton unloaded.");
                toggleButtonUnloadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = cmdBar;
        });

        buttonLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            buttonLabel = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(button, L"OverflowTextLabel"));
            originalAppBarButtonMargin = buttonLabel->Margin.Left;
            cmdBar->IsOpen = false;
        });

        buttonUnloadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->SecondaryCommands->Append(toggleButton);
            cmdBar->IsOpen = true;
        });

        buttonLoadedEvent->WaitForDefault();
        toggleButtonLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            buttonLabel = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(button, L"OverflowTextLabel"));
            VERIFY_IS_GREATER_THAN(buttonLabel->Margin.Left, originalAppBarButtonMargin);
            cmdBar->IsOpen = false;
        });

        buttonUnloadedEvent->WaitForDefault();
        toggleButtonUnloadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->SecondaryCommands->RemoveAt(1);
            cmdBar->IsOpen = true;
        });

        buttonLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            buttonLabel = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(button, L"OverflowTextLabel"));
            VERIFY_ARE_EQUAL(buttonLabel->Margin.Left, originalAppBarButtonMargin);
        });
    }

    void CommandBarIntegrationTests::ValidateInlineCommandBarLightDismissBehavior()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ tapTarget = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;

        auto clickEvent = std::make_shared<Event>();
        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();

        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);

        RunOnUIThread([&]()
        {
            tapTarget = ref new xaml_controls::Button();
            tapTarget->Content = "Click Me!";

            // Add a top margin to push the button out from under the statusbar on phone
            // and a bottom margin to make sure the CommandBar doesn't open over the button.
            tapTarget->Margin = xaml::ThicknessHelper::FromLengths(0, 32, 0, 32);

            cmdBar = ref new xaml_controls::CommandBar();
            clickRegistration.Attach(tapTarget, [&]() { clickEvent->Set(); });
            openedRegistration.Attach(cmdBar, [&]() { openedEvent->Set(); });
            closedRegistration.Attach(cmdBar, [&]() { closedEvent->Set(); });

            auto root = ref new xaml_controls::StackPanel();
            root->Children->Append(tapTarget);
            root->Children->Append(cmdBar);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = true;
        });
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Click outside of the CommandBar to close it.
        TestServices::InputHelper->Tap(tapTarget);
        closedEvent->WaitForDefault();

        // Validate that sticky CommandBars are not light-dismissible.
        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = true;
            cmdBar->IsSticky = true;
        });
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Click outside of the CommandBar.
        TestServices::InputHelper->Tap(tapTarget);
        TestServices::WindowHelper->WaitForIdle();

        // Since the CommandBar shouldn't be light-dismissible, the button should
        // have received the input and invoked its click handler.
        clickEvent->WaitForDefault();
    }

    void CommandBarIntegrationTests::ValidateRightClickBehavior()
    {
        // Leak: CommandBar leaks 102KB in ValidateRightClickBehavior test
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        LOG_OUTPUT(L"Validating CommandBarRightClickBehavior with ClosedDisplayMode=Hidden.");
        ValidateRightClickBehaviorWorker(xaml_controls::AppBarClosedDisplayMode::Hidden);

        LOG_OUTPUT(L"Validating CommandBarRightClickBehavior with ClosedDisplayMode=Minimal.");
        ValidateRightClickBehaviorWorker(xaml_controls::AppBarClosedDisplayMode::Minimal);

        LOG_OUTPUT(L"Validating CommandBarRightClickBehavior with ClosedDisplayMode=Compact.");
        ValidateRightClickBehaviorWorker(xaml_controls::AppBarClosedDisplayMode::Compact);
    }

    void CommandBarIntegrationTests::ValidateRightClickBehaviorWorker(xaml_controls::AppBarClosedDisplayMode closedDisplayMode)
    {
        // CoreWindow isn't agile, so we can't use the SafeEventRegistration utility,
        // so we have to manage it manually.
        wf::EventRegistrationToken coreWindowPointerPressedToken = {};

        TestCleanupWrapper cleanup([&coreWindowPointerPressedToken]()
        {
            RunOnUIThread([&coreWindowPointerPressedToken]()
            {
                xaml::Window::Current->CoreWindow->PointerPressed -= coreWindowPointerPressedToken;
                coreWindowPointerPressedToken = {};
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        // CommandBars should never open in response to right-click.
        const bool expectedTopBottomIsOpenValue = false;
        const bool expectedInlineIsOpenValue = false;

        xaml_controls::Page^ page = nullptr;
        xaml_controls::CommandBar^ topCmdBar = nullptr;
        xaml_controls::CommandBar^ bottomCmdBar = nullptr;
        xaml_controls::CommandBar^ inlineCmdBar = nullptr;

        auto rightClickProcessedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            topCmdBar = ref new xaml_controls::CommandBar();
            bottomCmdBar = ref new xaml_controls::CommandBar();
            inlineCmdBar = ref new xaml_controls::CommandBar();

            topCmdBar->ClosedDisplayMode = closedDisplayMode;
            bottomCmdBar->ClosedDisplayMode = closedDisplayMode;
            inlineCmdBar->ClosedDisplayMode = closedDisplayMode;

            auto grid = ref new xaml_controls::Grid();
            grid->Children->Append(inlineCmdBar);

            coreWindowPointerPressedToken = xaml::Window::Current->CoreWindow->PointerPressed +=
                ref new wf::TypedEventHandler<wuc::CoreWindow^, wuc::PointerEventArgs^>([rightClickProcessedEvent](wuc::CoreWindow^, wuc::PointerEventArgs^) {
                    rightClickProcessedEvent->Set();
                });

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            auto frame = safe_cast<xaml_controls::Frame^>(Window::Current->Content);

            page->TopAppBar = topCmdBar;
            page->BottomAppBar = bottomCmdBar;
            page->Content = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Inject right-click.
        TestServices::InputHelper->ClickMouseButton(MouseButton::Right, page);
        TestServices::WindowHelper->WaitForIdle();
        rightClickProcessedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(topCmdBar->IsOpen, expectedTopBottomIsOpenValue);
            VERIFY_ARE_EQUAL(bottomCmdBar->IsOpen, expectedTopBottomIsOpenValue);
            VERIFY_ARE_EQUAL(inlineCmdBar->IsOpen, expectedInlineIsOpenValue);
        });
        TestServices::WindowHelper->WaitForIdle();

        EmptyPageContent(page);
    }

    void CommandBarIntegrationTests::ValidateArrowKeys()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = nullptr;
        xaml_controls::Button^ moreButton = nullptr;

        Platform::String^ focusSequence = "";
        Platform::String^ expectedFocusSequence = "[M][P2][P1][P2][M][S1][S2][S4][M][S4][S2][S1][M]";

        std::vector<SafeEventRegistrationType(xaml_controls::AppBarButton, GotFocus)> buttonGotFocusRegistrations;
        auto separatorGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarSeparator, GotFocus);
        auto toggleButtonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarToggleButton, GotFocus);
        auto moreButtonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        Platform::String^ rightKeySequence = "#$d$_right#$u$_right";
        Platform::String^ leftKeySequence = "#$d$_left#$u$_left";
        Platform::String^ returnKeySequence = "#$d$_return#$u$_return";

        unsigned int primaryCount = 0;
        unsigned int secondaryCount = 0;

        xaml::RoutedEventHandler^ gotFocusHandler = nullptr;

        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();

            gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(sender)->Tag + "]";
            });

            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->IsOpen = true;

            // Add a couple of AppBarButtons to primary
            for (int i = 1; i <= 2; i++)
            {
                auto appBarButton = ref new xaml_controls::AppBarButton();
                appBarButton->Tag = "P" + i;
                cmdBar->PrimaryCommands->Append(appBarButton);

                auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, GotFocus);
                gotFocusRegistration.Attach(appBarButton, gotFocusHandler);
                buttonGotFocusRegistrations.push_back(std::move(gotFocusRegistration));
            }

            // Add a couple of AppBarButtons to secondary
            for (int i = 1; i <= 2; i++)
            {
                auto appBarButton = ref new xaml_controls::AppBarButton();
                appBarButton->Tag = "S" + i;
                cmdBar->SecondaryCommands->Append(appBarButton);

                auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, GotFocus);
                gotFocusRegistration.Attach(appBarButton, gotFocusHandler);
                buttonGotFocusRegistrations.push_back(std::move(gotFocusRegistration));
            }

            // Add an AppBarSeparator to secondary
            {
                auto appBarSeparator = ref new xaml_controls::AppBarSeparator();
                appBarSeparator->Tag = "S3";
                cmdBar->SecondaryCommands->Append(appBarSeparator);
                separatorGotFocusRegistration.Attach(appBarSeparator, gotFocusHandler);
            }

            // Add an AppBarToggleButton to secondary
            {
                auto appBarToggleButton = ref new xaml_controls::AppBarToggleButton();
                appBarToggleButton->Tag = "S4";
                cmdBar->SecondaryCommands->Append(appBarToggleButton);
                toggleButtonGotFocusRegistration.Attach(appBarToggleButton, gotFocusHandler);
            }
            page->BottomAppBar = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton"));
            VERIFY_IS_NOT_NULL(moreButton);
            moreButton->Tag = "M";
            moreButtonGotFocusRegistration.Attach(moreButton, gotFocusHandler);

            primaryCount = cmdBar->PrimaryCommands->Size;
            secondaryCount = cmdBar->SecondaryCommands->Size;
        });
        TestServices::WindowHelper->WaitForIdle();

        focusSequence = "";

        // Start focus with more button
        RunOnUIThread([&]()
        {
            moreButton->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Press left arrow key (number of primary commands + 1) times
        for (unsigned int i = 0; i <= primaryCount; i++)
        {
            TestServices::KeyboardHelper->PressKeySequence(leftKeySequence);
            TestServices::WindowHelper->WaitForIdle();
        }

        // Press right arrow key (number of primary commands + 1) times
        for (unsigned int i = 0; i <= primaryCount; i++)
        {
            TestServices::KeyboardHelper->PressKeySequence(rightKeySequence);
            TestServices::WindowHelper->WaitForIdle();
        }

        // Press down arrow key (number of secondary commands - 1) times
        for (unsigned int i = 0; i < secondaryCount; i++)
        {
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();
        }

        // Press up arrow key (number of secondary commands - 1) times
        for (unsigned int i = 0; i < secondaryCount; i++)
        {
            TestServices::KeyboardHelper->Up();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence:   %s", (focusSequence)->Data());
        VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);

        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });
    }

    void CommandBarIntegrationTests::ValidateUIElementTreeBoth()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating CommandBars with both Primary & Secondary commands.");
        ControlHelper::ValidateUIElementTree(
            ValidateTreeParams(
                PopupHelper::AreWindowedPopupsEnabled() ? L"Windowed" : L"Unwindowed",
                wf::Size(500, 800),
                1.f,
                []()
                {
                    return ValidateUIElementTestSetup(true /*addPrimary*/, true /*addSecondary*/);
                },
                GetUIElementTreeValidationRules())
            );
    }

    void CommandBarIntegrationTests::ValidateUIElementTreePrimaryOnly()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating CommandBars with only Primary commands.");
        ControlHelper::ValidateUIElementTree(
            ValidateTreeParams(
                PopupHelper::AreWindowedPopupsEnabled() ? L"Windowed" : L"Unwindowed",
                wf::Size(500, 800),
                1.f,
                []()
                {
                    return ValidateUIElementTestSetup(true /*addPrimary*/, false /*addSecondary*/);
                },
                GetUIElementTreeValidationRules())
            );
    }

    void CommandBarIntegrationTests::ValidateUIElementTreeSecondaryOnly()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating CommandBars with only Secondary commands.");
        ControlHelper::ValidateUIElementTree(
                ValidateTreeParams(
                PopupHelper::AreWindowedPopupsEnabled() ? L"Windowed" : L"Unwindowed",
                wf::Size(500, 800),
                1.f,
                []()
                {
                    return ValidateUIElementTestSetup(false /*addPrimary*/, true /*addSecondary*/);
                },
                GetUIElementTreeValidationRules())
                );
    }

    xaml_controls::Panel^ CommandBarIntegrationTests::ValidateUIElementTestSetup(bool addPrimary, bool addSecondary)
    {
        xaml_controls::Grid^ rootGrid = nullptr;
        RunOnUIThread([&]()
        {
            rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Inject a tab to ensure that the last input device type used is consistent,
        // since that affects the layout of the CommandBar.
        TestServices::KeyboardHelper->Tab();

        xaml_controls::AppBarButton^ lastAddedButton = nullptr;

        RunOnUIThread([&]()
        {
            const size_t numClosedDisplayModes = 3;
            const size_t numDefaultLabelPositions = 3;

            for (size_t mode = 0; mode < numClosedDisplayModes; ++mode)
            {
                for (size_t isOpen = 0; isOpen < 2; ++isOpen)
                {
                    for (size_t defaultLabelPosition = 0; defaultLabelPosition < numDefaultLabelPositions; ++defaultLabelPosition)
                    {
                        auto cmdBar = ref new xaml_controls::CommandBar();
                        cmdBar->IsOpen = (isOpen > 0);
                        cmdBar->ClosedDisplayMode = static_cast<xaml_controls::AppBarClosedDisplayMode>(mode);
                        cmdBar->DefaultLabelPosition = static_cast<xaml_controls::CommandBarDefaultLabelPosition>(defaultLabelPosition);

                        if (addPrimary)
                        {
                            lastAddedButton = ref new xaml_controls::AppBarButton();
                            lastAddedButton->Label = "button";
                            cmdBar->PrimaryCommands->Append(lastAddedButton);
                        }

                        if (addSecondary)
                        {
                            lastAddedButton = ref new xaml_controls::AppBarButton();
                            lastAddedButton->Label = "button";
                            cmdBar->SecondaryCommands->Append(lastAddedButton);
                        }

                        rootGrid->Children->Append(cmdBar);
                        xaml_controls::Grid::SetRow(cmdBar, 2 * static_cast<int>(mode) + static_cast<int>(isOpen));
                        xaml_controls::Grid::SetColumn(cmdBar, static_cast<int>(defaultLabelPosition));
                    }
                }
            }

            for (size_t rowCount = 0; rowCount < 2 * numClosedDisplayModes; rowCount++)
            {
                rootGrid->RowDefinitions->Append(ref new xaml_controls::RowDefinition());
            }

            for (size_t columnCount = 0; columnCount < numDefaultLabelPositions; columnCount++)
            {
                rootGrid->ColumnDefinitions->Append(ref new xaml_controls::ColumnDefinition());
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        // Set focus on a primary button or a secondary button because having focus present on the MoreButton (by default)
        // causes the "More options" ToolTip to appear depending upon timing and cause unpredictable failures.
        RunOnUIThread([&]()
        {
            lastAddedButton->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        return rootGrid;
    }

    void CommandBarIntegrationTests::PrimaryCommandItemsDoNotDisappear()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            auto rootGrid = ref new xaml_controls::Grid();

            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Center; // Center it to get it out from under the statusbar.

            auto appBarButton = ref new xaml_controls::AppBarButton();
            appBarButton->Label = "button";
            cmdBar->PrimaryCommands->Append(appBarButton);

            rootGrid->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Change the ClosedDisplayMode to Hidden.
        RunOnUIThread([&]() { cmdBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Hidden; });
        TestServices::WindowHelper->WaitForIdle();

        // Open the menu.
        RunOnUIThread([&]() { cmdBar->IsOpen = true; });
        TestServices::WindowHelper->WaitForIdle();

        // Close the menu.
        RunOnUIThread([&]() { cmdBar->IsOpen = false; });
        TestServices::WindowHelper->WaitForIdle();

        // Change the ClosedDisplayMode back to Compact.
        RunOnUIThread([&]() { cmdBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Compact; });
        TestServices::WindowHelper->WaitForIdle();

        // Attempt to tap the more button.
        xaml_controls::Button^ moreButton = nullptr;
        auto clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        // Find the MoreButton and attempt to tap it.
        RunOnUIThread([&]()
        {
            moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton"));
            clickRegistration.Attach(moreButton, [&]() { clickEvent->Set(); });
        });

        TestServices::InputHelper->Tap(moreButton);
        clickEvent->WaitForDefault();
    }

    void CommandBarIntegrationTests::ValidateOverflowScrollViewerDoesNotScrollWithArrowKeys()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        wf::Point firstItemOriginalPosition = {};

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->IsOpen = true;

            for (size_t i = 0; i < 50; ++i)
            {
                auto button = ref new xaml_controls::AppBarButton();
                button->Label = "menu item";

                cmdBar->SecondaryCommands->Append(button);
            }

            TestServices::WindowHelper->WindowContent = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus the first element in the overflow menu.
            auto item = safe_cast<xaml_controls::AppBarButton^>(cmdBar->SecondaryCommands->GetAt(0));
            item->Focus(xaml::FocusState::Keyboard);

            // Save off its original position.
            auto transform = item->TransformToVisual(nullptr);
            firstItemOriginalPosition = transform->TransformPoint(wf::Point(0, 0));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Press down arrow key
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto item = safe_cast<xaml_controls::AppBarButton^>(cmdBar->SecondaryCommands->GetAt(0));
            auto transform = item->TransformToVisual(nullptr);
            auto firstItemNewPosition = transform->TransformPoint(wf::Point(0, 0));

            VERIFY_ARE_EQUAL(firstItemNewPosition, firstItemOriginalPosition);
        });
    }

    void CommandBarIntegrationTests::DoesFocusReturnToMoreButtonFromOverflowMenuWhenClosed()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->IsOpen = true;

            auto button = ref new xaml_controls::AppBarButton();
            cmdBar->SecondaryCommands->Append(button);

            TestServices::WindowHelper->WindowContent = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Focus the first element in the overflow menu.
            auto item = safe_cast<xaml_controls::AppBarButton^>(cmdBar->SecondaryCommands->GetAt(0));
            item->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
            auto moreButton = TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton");

            VERIFY_IS_TRUE(focusedElement->Equals(moreButton));
        });
    }

    void CommandBarIntegrationTests::ValidateFirstElementIsNotFocusedWhenClosingCommandBar()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;
        auto textBoxWasFocused = std::make_shared<bool>();
        auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->PrimaryCommands->Append(ref new xaml_controls::AppBarButton());

            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = cmdBar;

            // Add a TextBox and a Button to the page with the TextBox being the First Focusable Element.
            auto stackPanel = ref new xaml_controls::StackPanel();
            textBox = ref new xaml_controls::TextBox();
            stackPanel->Children->Append(textBox);
            button = ref new xaml_controls::Button();
            button->Content = L"Button";
            stackPanel->Children->Append(button);
            page->Content = stackPanel;

            textBoxGotFocusRegistration.Attach(textBox, [&]() { *textBoxWasFocused = true; });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Move focus to Button so it becomes the Previously Focused Element.
            button->Focus(FocusState::Programmatic);
            cmdBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        *textBoxWasFocused = false;

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the focus did not move to the First Focusable Element.
        VERIFY_IS_FALSE(*textBoxWasFocused);
    }

    void CommandBarIntegrationTests::CanMaintainFocusAfterCollectionOrSizeChange()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ commandBar = nullptr;

        auto addButton = [](Platform::String^ label, wfc::IObservableVector<xaml_controls::ICommandBarElement^>^ commands) {
            auto button = ref new xaml_controls::AppBarButton();
            button->Label = label;
            commands->Append(button);
        };

        RunOnUIThread([&]()
        {
            commandBar = ref new xaml_controls::CommandBar();
            commandBar->IsDynamicOverflowEnabled = false;
            commandBar->IsOpen = true;
            commandBar->IsSticky = true;
            commandBar->Width = 500.0;

            for (int i = 0; i < 3; ++i)
            {
                addButton(L"Primary Item #" + i.ToString(), commandBar->PrimaryCommands);
                addButton(L"Secondary Item #" + i.ToString(), commandBar->SecondaryCommands);
            }

            TestServices::WindowHelper->WindowContent = commandBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Focus the second primary command.");
            safe_cast<xaml_controls::Control^>(commandBar->PrimaryCommands->GetAt(1))->Focus(xaml::FocusState::Keyboard);

            LOG_OUTPUT(L"Add new primary command.");
            addButton(L"Added Primary Item", commandBar->PrimaryCommands);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate the second primary command still has focus.");
            VERIFY_ARE_EQUAL(commandBar->PrimaryCommands->GetAt(1), safe_cast<xaml_controls::ICommandBarElement^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));

            LOG_OUTPUT(L"Focus the second secondary command.");
            safe_cast<xaml_controls::Control^>(commandBar->SecondaryCommands->GetAt(1))->Focus(xaml::FocusState::Keyboard);

            LOG_OUTPUT(L"Add new secondary command.");
            addButton(L"Added Secondary Item", commandBar->SecondaryCommands);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate the second secondary command still has focus.");
            VERIFY_ARE_EQUAL(commandBar->SecondaryCommands->GetAt(1), safe_cast<xaml_controls::ICommandBarElement^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));

            LOG_OUTPUT(L"Clearing all secondary commands. Focus is expected to go to the more button.");
            commandBar->SecondaryCommands->Clear();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate the more button has focus.");
            auto moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(commandBar, L"MoreButton"));
            VERIFY_ARE_EQUAL(moreButton, safe_cast<xaml_controls::Button^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));

            LOG_OUTPUT(L"Focus fourth primary command, enable dynamic overflow and resize command bar.");
            safe_cast<xaml_controls::Control^>(commandBar->PrimaryCommands->GetAt(3))->Focus(xaml::FocusState::Keyboard);
            commandBar->IsDynamicOverflowEnabled = true;
            commandBar->Width = 250;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Fourth primary command is now in the secondary ItemsControl. Validate it still has focus.");
            VERIFY_ARE_EQUAL(commandBar->PrimaryCommands->GetAt(3), safe_cast<xaml_controls::ICommandBarElement^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));

            LOG_OUTPUT(L"Resize command bar back to its original size.");
            commandBar->Width = 500;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Fourth primary command is back in the primary ItemsControl. Validate it still has focus.");
            VERIFY_ARE_EQUAL(commandBar->PrimaryCommands->GetAt(3), safe_cast<xaml_controls::ICommandBarElement^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
        });
    }

    void CommandBarIntegrationTests::CanReopenInClosedHandler()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ appBarButton = nullptr;
        auto commandBarClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            appBarButton = ref new xaml_controls::AppBarButton();
            cmdBar->PrimaryCommands->Append(appBarButton);
            commandBarClosedRegistration.Attach(cmdBar, ref new wf::EventHandler<Platform::Object^>([](Platform::Object^ sender, Platform::Object^ args)
            {
                safe_cast<xaml_controls::CommandBar^>(sender)->IsOpen = true;
            }));

            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(cmdBar->IsOpen);
            VERIFY_IS_FALSE(appBarButton->IsCompact);

            auto overflowPopup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(cmdBar, L"OverflowPopup"));

            VERIFY_IS_TRUE(overflowPopup->IsOpen);

            // We need to close the popup at the end of this test,
            // and without detaching our event handler, closing it
            // is just going to cause it to re-open again.
            // So we need to detach the event handler here.
            commandBarClosedRegistration.Detach();

            cmdBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::CanNotTabIntoWhenClosedAndHidden()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;

        auto cmdBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, GotFocus);
        bool didCmdBarGetFocus = false;

        RunOnUIThread([&]()
        {
            auto cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Hidden;

            // Add some content to make sure it doesn't get focus either.
            cmdBar->PrimaryCommands->Append(ref new xaml_controls::AppBarButton());

            cmdBarGotFocusRegistration.Attach(cmdBar, [&]() { didCmdBarGetFocus = true; });

            button = ref new xaml_controls::Button();
            button->Content = L"button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(button);
            root->Children->Append(cmdBar);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(didCmdBarGetFocus);
    }

    void CommandBarIntegrationTests::DoesNotShowMenuIfSecondaryElementsAreCollapsed()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);

        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            auto collapsedAppBarButton = ref new xaml_controls::AppBarButton();
            collapsedAppBarButton->Visibility = xaml::Visibility::Collapsed;

            auto collapsedAppBarSeparator = ref new xaml_controls::AppBarSeparator();
            collapsedAppBarSeparator->Visibility = xaml::Visibility::Collapsed;

            auto collapsedAppBarToggleButton = ref new xaml_controls::AppBarButton();
            collapsedAppBarToggleButton->Visibility = xaml::Visibility::Collapsed;

            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->IsOpen = true;
            cmdBar->SecondaryCommands->Append(collapsedAppBarButton);
            cmdBar->SecondaryCommands->Append(collapsedAppBarSeparator);
            cmdBar->SecondaryCommands->Append(collapsedAppBarToggleButton);

            loadedRegistration.Attach(cmdBar, [&]()
            {
                LOG_OUTPUT(L"CommandBar.Loaded raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = cmdBar;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto overflowContentRoot = safe_cast<xaml::UIElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar));
            VERIFY_IS_NOT_NULL(overflowContentRoot);

            VERIFY_ARE_EQUAL(overflowContentRoot->Visibility, xaml::Visibility::Collapsed);
        });
    }

    void CommandBarIntegrationTests::ValidateInlineCommandBarOpenDirection(xaml::VerticalAlignment alignment)
    {
        xaml_controls::CommandBar^ commandBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            commandBar = ref new xaml_controls::CommandBar();
            commandBar->VerticalAlignment = alignment;
            commandBar->PrimaryCommands->Append(ref new xaml_controls::AppBarButton());
            commandBar->SecondaryCommands->Append(ref new xaml_controls::AppBarButton());
            commandBar->SecondaryCommands->Append(ref new xaml_controls::AppBarButton());
            xaml_controls::Grid^ grid = ref new xaml_controls::Grid();
            grid->Children->Append(commandBar);
            page->Content = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        xaml::UIElement^ overflowContentRoot;

        RunOnUIThread([&]()
        {
            overflowContentRoot = safe_cast<xaml::UIElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", commandBar));
        });
        TestServices::WindowHelper->WaitForIdle();

        wf::Rect commandBarBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(commandBar));
        wf::Rect commandBarOverflowBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(overflowContentRoot));

        LOG_OUTPUT(L"commandBarBounds:         (%f, %f, %f, %f)", commandBarBounds.X, commandBarBounds.Y, commandBarBounds.Width, commandBarBounds.Height);
        LOG_OUTPUT(L"commandBarOverflowBounds: (%f, %f, %f, %f)", commandBarOverflowBounds.X, commandBarOverflowBounds.Y, commandBarOverflowBounds.Width, commandBarOverflowBounds.Height);

        //Test that the CommandBarOverflow is positioned correctly
        switch (alignment)
        {
            case Microsoft::UI::Xaml::VerticalAlignment::Top:
                VERIFY_IS_TRUE(commandBarOverflowBounds.Y >= commandBarBounds.Y);
                break;
            case Microsoft::UI::Xaml::VerticalAlignment::Center:
                VERIFY_IS_TRUE(commandBarOverflowBounds.Y >= commandBarBounds.Y);
                break;
            case Microsoft::UI::Xaml::VerticalAlignment::Bottom:
                VERIFY_IS_TRUE(commandBarOverflowBounds.Y < commandBarBounds.Y);
                break;
        }
    }

    void CommandBarIntegrationTests::ValidateCommandBarOpensInsideLayoutBounds()
    {
        TestCleanupWrapper cleanup;

        ValidateInlineCommandBarOpenDirection(xaml::VerticalAlignment::Top);
        ValidateInlineCommandBarOpenDirection(xaml::VerticalAlignment::Center);
        ValidateInlineCommandBarOpenDirection(xaml::VerticalAlignment::Bottom);
    }

    void CommandBarIntegrationTests::DoesCloseBeforeButtonClickHandlersExecute()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ appBarButton = nullptr;

        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, Click);
        auto clickEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            appBarButton = ref new xaml_controls::AppBarButton();

            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->PrimaryCommands->Append(appBarButton);
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Center;
            cmdBar->IsOpen = true;

            clickRegistration.Attach(appBarButton, ref new xaml::RoutedEventHandler([cmdBar, clickEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                // The CommandBar should be closing before this handler executes.
                VERIFY_IS_FALSE(cmdBar->IsOpen);

                clickEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(appBarButton);
        clickEvent->WaitForDefault();
    }

    void CommandBarIntegrationTests::DoesCycleFocusWhenOpen()
    {
        auto expectedFocusSequence = L"[S1][P1][P2][P3][M][P3][P2][P1][S1][M]";
        DoesCycleFocusWhenOpenWorker(Location::Inline, 5, expectedFocusSequence);
        DoesCycleFocusWhenOpenWorker(Location::Top, 5, expectedFocusSequence);
        DoesCycleFocusWhenOpenWorker(Location::Bottom, 5, expectedFocusSequence);
    }

    void CommandBarIntegrationTests::DoesCycleFocusWhenOpenWorker(Location location, size_t numTabs, const wchar_t* expectedFocusSequence)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;
        xaml_controls::StackPanel^ panel = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;

        auto pageLoadedEvent = std::make_shared<Event>();
        auto moreButtonGotFocusEvent = std::make_shared<Event>();
        auto pageLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto moreButtonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto commandBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, GotFocus);
        auto overflowGotFocusRegistration = CreateSafeEventRegistration(xaml::UIElement, GotFocus);

        Platform::String^ focusSequence = "";
        xaml::RoutedEventHandler^ gotFocusHandler = nullptr;

        RunOnUIThread([&]()
        {
            gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^, xaml::RoutedEventArgs^ args)
            {
                focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Tag + "]";
            });

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            pageLoadedRegistration.Attach(page, [&]{ pageLoadedEvent->Set(); });

            panel = ref new xaml_controls::StackPanel();

            auto button = ref new xaml_controls::Button();
            button->Content = "Button";
            button->Tag = "B";
            buttonGotFocusRegistration.Attach(button, gotFocusHandler);
            panel->Children->Append(button);

            cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                        <AppBarButton Tag="P1"/>
                        <AppBarButton Tag="P2"/>
                        <AppBarButton Tag="P3"/>
                        <CommandBar.SecondaryCommands>
                            <AppBarButton Tag="S1"/>
                            <AppBarButton Tag="S2"/>
                            <AppBarButton Tag="S3"/>
                        </CommandBar.SecondaryCommands>
                    </CommandBar>)"));
            commandBarGotFocusRegistration.Attach(cmdBar, gotFocusHandler);

            switch (location)
            {
            case Location::Inline:
                panel->Children->Append(cmdBar);
                break;
            case Location::Top:
                page->TopAppBar = cmdBar;
                break;
            case Location::Bottom:
                page->BottomAppBar = cmdBar;
                break;
            }

            page->Content = panel;
        });
        pageLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Start with the focus on the moreButton.
            auto moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton"));
            moreButton->Tag = "M";
            moreButtonGotFocusRegistration.Attach(moreButton, [&]{ moreButtonGotFocusEvent->Set(); });
            moreButton->Focus(FocusState::Keyboard);

            cmdBar->IsOpen = true;
        });
        moreButtonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        focusSequence = "";

        RunOnUIThread([&]()
        {
            auto overflowContentRoot = safe_cast<xaml::UIElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", panel));
            overflowGotFocusRegistration.Attach(overflowContentRoot, gotFocusHandler);
        });

        // Tab several times to cycle focus through the CommandBar.
        for (size_t i = 0; i < numTabs; ++i)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        // Shift-Tab several times to cycle focus through the CommandBar in reverse.
        for (size_t i = 0; i < numTabs; ++i)
        {
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence);
        LOG_OUTPUT(L"Actual focus sequence: %s", (focusSequence)->Data());
        VERIFY_ARE_EQUAL(ref new Platform::String(expectedFocusSequence), focusSequence);

        EmptyPageContent(page);
    }

    void CommandBarIntegrationTests::CanTabIntoOverflowMenuWhenTopOrBottom()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ appBarButton = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            appBarButton = ref new xaml_controls::AppBarButton();

            cmdBar->SecondaryCommands->Append(appBarButton);

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = cmdBar;

            TestServices::WindowHelper->WindowContent = page;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Tab once to move into the overflow menu.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(appBarButton));

            cmdBar->IsOpen = false;

            page->TopAppBar = nullptr;
            page->BottomAppBar = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Tab once to move into the overflow menu.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(appBarButton));

            cmdBar->IsOpen = false;
        });
    }

    void CommandBarIntegrationTests::ValidateClosedMinimalCommandBarWithSecondaryCommandsOnlyIsVisible()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ commandBar = nullptr;

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);

        xaml::FrameworkElement^ moreButton = nullptr;

        RunOnUIThread([&]()
        {
            commandBar = ref new xaml_controls::CommandBar();
            commandBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Minimal;
            commandBar->VerticalAlignment = xaml::VerticalAlignment::Center; // Center it to get it out from under the status bar.
            openedRegistration.Attach(commandBar, [&]() { openedEvent->Set(); });

            // Add secondary commands
            auto appBarButton = ref new xaml_controls::AppBarButton();
            commandBar->SecondaryCommands->Append(appBarButton);

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(commandBar);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Try tapping the more button
        RunOnUIThread([&]()
        {
            moreButton = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByName(commandBar, L"MoreButton"));
        });
        TestServices::InputHelper->Tap(moreButton);
        openedEvent->WaitForDefault();
    }

    //--------------------------------------------------------------------------------------
    // Test case: Verifies that once you enter a bottom commandbar overflow,
    //            you can exit it by pressing Escape when the focus is on an overflow item
    //--------------------------------------------------------------------------------------
    void CommandBarIntegrationTests::DoesFocusExpandButtonWithOverflowEscKey()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 800));

        auto rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetPackageFolder()
            + L"resources\\native\\controls\\commandbar\\CommandBarTest.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);

        RunOnUIThread([&]()
        {
            loadedEventRegistration.Attach(rootPage, [&]
            {
                LOG_OUTPUT(L"Page.Loaded event raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPage;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::CommandBar^ commandBar = nullptr;
        xaml_controls::Button^ moreButton = nullptr;

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto openedEvent = std::make_shared<Event>();

        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto closedEvent = std::make_shared<Event>();

        auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
        auto gotFocusEvent = std::make_shared<Event>();        

        RunOnUIThread([&]()
        {
            commandBar = safe_cast<xaml_controls::CommandBar^>(rootPage->Content);
            VERIFY_IS_NOT_NULL(commandBar);

            moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(commandBar, L"MoreButton"));
            VERIFY_IS_NOT_NULL(moreButton);

            openedRegistration.Attach(commandBar, [&]
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Opened Event Fired");
                    openedEvent->Set();
                });

            closedRegistration.Attach(commandBar, [&]
                {
                    LOG_OUTPUT(L"[bottomCommandBar]: Closed Event Fired");
                    closedEvent->Set();
                });

            auto overflowAppBarBtn0 = safe_cast<xaml_controls::AppBarButton^>(commandBar->PrimaryCommands->GetAt(5));
            VERIFY_IS_NOT_NULL(overflowAppBarBtn0);

            gotFocusRegistration.Attach(overflowAppBarBtn0, [&]
                {
                    LOG_OUTPUT(L"[bottomOverflowAppBarBtn0]: Got Focus Event Fired");
                    gotFocusEvent->Set();
                });
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting keyboard focus on MoreButton.");
            moreButton->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking MoreButton to open overflow.");
        TestServices::KeyboardHelper->Enter();
        openedEvent->WaitForDefault();
        gotFocusEvent->WaitForDefault();        
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Escape(); //Close overflow
        closedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(moreButton->FocusState == FocusState::Keyboard);
        });
    }

    void CommandBarIntegrationTests::ValidateMenuSizingForDifferentInputModes()
    {
        TestCleanupWrapper cleanup;

        // The size of the CommandBar menu and the size of the items in it (AppBarButton/AppBarToggleButton) change
        // based on whether the CommandBar was opened via Touch or not.
        // We test opening the CommandBar using Touch, Mouse, Keyboard and Programmatically and validate that the
        // menu and its items size correctly in each case.

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

        // The expected sizes of the menu/items.
        // For the first two we can read the expected values from generic.xaml. For the latter two, there is no corresponding resource
        // so we just hard-code the values here.
        double expectedMenuWidth_Touch;
        double expectedMenuWidth_NonTouch;
        const double expectedMenuItemHeight_Touch = 40;
        const double expectedMenuItemHeight_NonTouch = 32;

        xaml_controls::CommandBar^ cmdBar;
        xaml_controls::AppBarButton^ appBarButton;
        xaml_controls::AppBarToggleButton^ appBarToggleButton;
        xaml::FrameworkElement^ overflowContentRoot;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <CommandBar x:Name="cmdBar" VerticalAlignment="Bottom" HorizontalAlignment="Center" >
                            <CommandBar.SecondaryCommands>
                                <AppBarButton x:Name="appBarButton" Label="Item 1" />
                                <AppBarToggleButton x:Name="appBarToggleButton" Label="Item 2" />
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                    </Grid>)"));

            cmdBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"cmdBar"));
            appBarButton = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"appBarButton"));
            appBarToggleButton = safe_cast<xaml_controls::AppBarToggleButton^>(root->FindName(L"appBarToggleButton"));

            expectedMenuWidth_Touch = safe_cast<double>(root->Resources->Lookup(L"CommandBarOverflowTouchMinWidth"));
            expectedMenuWidth_NonTouch = safe_cast<double>(root->Resources->Lookup(L"CommandBarOverflowMinWidth"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open via Touch:
        OpenCommandBar(cmdBar, OpenMethod::Touch);
        RunOnUIThread([&]()
        {
            overflowContentRoot = TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar);

            VERIFY_ARE_EQUAL(expectedMenuWidth_Touch, overflowContentRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedMenuWidth_Touch, overflowContentRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_Touch, appBarButton->ActualHeight);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_Touch, appBarToggleButton->ActualHeight);
        });
        CloseCommandBar(cmdBar);

        // Open via Gamepad:
        OpenCommandBar(cmdBar, OpenMethod::Gamepad);
        RunOnUIThread([&]()
        {
            overflowContentRoot = TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar);

            VERIFY_ARE_EQUAL(expectedMenuWidth_Touch, overflowContentRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedMenuWidth_Touch, overflowContentRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_Touch, appBarButton->ActualHeight);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_Touch, appBarToggleButton->ActualHeight);
        });
        CloseCommandBar(cmdBar);

        // Open programmatically:
        // We expect a programmatically opened command bar to size based on the most recently used input mode (touch in this case)
        OpenCommandBar(cmdBar, OpenMethod::Programmatic);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedMenuWidth_Touch, overflowContentRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedMenuWidth_Touch, overflowContentRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_Touch, appBarButton->ActualHeight);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_Touch, appBarToggleButton->ActualHeight);
        });
        CloseCommandBar(cmdBar);

        // Open via Mouse:
        OpenCommandBar(cmdBar, OpenMethod::Mouse);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedMenuWidth_NonTouch, overflowContentRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedMenuWidth_NonTouch, overflowContentRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_NonTouch, appBarButton->ActualHeight);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_NonTouch, appBarToggleButton->ActualHeight);
        });
        CloseCommandBar(cmdBar);

        // Open via Keyboard:
        OpenCommandBar(cmdBar, OpenMethod::Keyboard);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedMenuWidth_NonTouch, overflowContentRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedMenuWidth_NonTouch, overflowContentRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_NonTouch, appBarButton->ActualHeight);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_NonTouch, appBarToggleButton->ActualHeight);
        });
        CloseCommandBar(cmdBar);

        // Open programmatically:
        // We expect a programmatically opened command bar to size based on the most recently used input mode (keyboard in this case)
        OpenCommandBar(cmdBar, OpenMethod::Programmatic);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedMenuWidth_NonTouch, overflowContentRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedMenuWidth_NonTouch, overflowContentRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_NonTouch, appBarButton->ActualHeight);
            VERIFY_ARE_EQUAL(expectedMenuItemHeight_NonTouch, appBarToggleButton->ActualHeight);
        });
        CloseCommandBar(cmdBar);
    }

    void CommandBarIntegrationTests::ValidateVisualStateForFullWidthMenu()
    {
        TestCleanupWrapper cleanup;

        // At small window sizes (<=480px wide), the CommandBar stretches to fill the full width of the window.
        // When in this mode, we do not draw a border on all 4 sides, instead we only draw one at the edge
        // opposite to the direction the CommandBar opens.
        //
        // We test this scenario for a CommandBar opening UP and opening DOWN.
        //
        // We validate:
        //   a. The value of BorderThickness on the overflow menu.
        //   b. The MinWidth, MaxWidth and ActualWidth of the overflow container are all equal to
        //      the window width.
        //   c. The VisualState of the CommandBarOverflowPresenter ("FullWidthOpenUp" or "FullWidthOpenDown").

        const float windowWidth = 300;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(windowWidth, 600));

        xaml_controls::CommandBar^ cmdBar;

        xaml::FrameworkElement^ overflowContentRoot; // The template part of the CommandBar that hosts the CommandBarOverFlowPresenter
        xaml_controls::CommandBarOverflowPresenter^ overflowPresenter; // The CommandBarOverflowPresenter
        xaml_controls::Grid^ overflowPresenterRoot; // The template root of the CommandBarOverflowPresenter. The Border is drawn on this element.

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <CommandBar x:Name="cmdBar" VerticalAlignment="Bottom" >
                            <CommandBar.SecondaryCommands>
                                <AppBarButton Label="Item 1" />
                                <AppBarToggleButton Label="Item 2" />
                                <AppBarButton Label="Item 3" />
                                <AppBarButton Label="Item 4" />
                                <AppBarButton Label="Item 5" />
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                    </Grid>)"));

            cmdBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"cmdBar"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // CommandBar starts aligned to the Bottom of the Window. It will open up.
        OpenCommandBar(cmdBar, OpenMethod::Programmatic);
        RunOnUIThread([&]()
        {
            overflowContentRoot = TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar);
            overflowPresenter = TreeHelper::GetVisualChildByType<xaml_controls::CommandBarOverflowPresenter>(overflowContentRoot);
            overflowPresenterRoot = safe_cast<xaml_controls::Grid^>(TreeHelper::GetVisualChildByName(overflowPresenter, L"LayoutRoot"));

            VERIFY_ARE_EQUAL(windowWidth, overflowContentRoot->ActualWidth);
            VERIFY_ARE_EQUAL(windowWidth, overflowContentRoot->MaxWidth);
            VERIFY_ARE_EQUAL(windowWidth, overflowContentRoot->MinWidth);
            VERIFY_ARE_EQUAL(windowWidth, overflowPresenter->ActualWidth);
            VERIFY_ARE_EQUAL(xaml::ThicknessHelper::FromLengths(0, 1, 0, 0), overflowPresenterRoot->BorderThickness);
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(overflowPresenter, L"DisplayModeStates", L"FullWidthOpenUp"));
        });
        CloseCommandBar(cmdBar);

        // Move the CommandBar to the Top of the Window so that it opens down.
        RunOnUIThread([&]()
        {
            cmdBar->VerticalAlignment = xaml::VerticalAlignment::Top;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenCommandBar(cmdBar, OpenMethod::Programmatic);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(windowWidth, overflowContentRoot->ActualWidth);
            VERIFY_ARE_EQUAL(windowWidth, overflowContentRoot->MaxWidth);
            VERIFY_ARE_EQUAL(windowWidth, overflowContentRoot->MinWidth);
            VERIFY_ARE_EQUAL(windowWidth, overflowPresenter->ActualWidth);
            VERIFY_ARE_EQUAL(xaml::ThicknessHelper::FromLengths(0, 0, 0, 1), overflowPresenterRoot->BorderThickness);
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(overflowPresenter, L"DisplayModeStates", L"FullWidthOpenDown"));
        });
        CloseCommandBar(cmdBar);
    }

    void CommandBarIntegrationTests::MoveItemsBetweenPrimaryAndSecondaryCommands()
    {
        TestCleanupWrapper cleanup;

        // AppBarButtons/AppBarToggleButtons get put into different visual states depending on whether
        // they are in PrimaryCommands or SecondayCommands.
        // Specifically:
        //   a. Items in SecondaryCommands should be in the Overflow state.
        //   b. If the SecondaryCommands contains both AppBarButtons and AppBarToggleButtons, the AppBarButtons should be
        //      in the OverflowWithToggleButtons state (instead of Overflow state).
        //   c. Items in SecondaryCommands should be in TouchInputMode state if the CommandBar was opened by touch.
        //   d. Items in the PrimaryCommands should never be in any of the above mentioned states. They should be in the
        //      "FullSize" (when the CommandBar is open) and "InputModeDefault" states.
        //
        // We verify that moving items between these two collections results in the items always being in the correct states.

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::CommandBar^ cmdBar;
        xaml_controls::AppBarButton^ appBarButton1;
        xaml_controls::AppBarButton^ appBarButton2;
        xaml_controls::AppBarToggleButton^ appBarToggleButton1;
        xaml_controls::AppBarToggleButton^ appBarToggleButton2;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <CommandBar x:Name="cmdBar" VerticalAlignment="Bottom" HorizontalAlignment="Center" >
                            <CommandBar.PrimaryCommands>
                                <AppBarButton x:Name="appBarButton1" Label="Item 1" />
                                <AppBarToggleButton x:Name="appBarToggleButton1" Label="Item 2" />
                            </CommandBar.PrimaryCommands>
                            <CommandBar.SecondaryCommands>
                                <AppBarButton x:Name="appBarButton2" Label="Item 4" />
                                <AppBarToggleButton x:Name="appBarToggleButton2" Label="Item 5" />
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                    </Grid>)"));

            cmdBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"cmdBar"));
            appBarButton1 = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"appBarButton1"));
            appBarButton2 = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"appBarButton2"));
            appBarToggleButton1 = safe_cast<xaml_controls::AppBarToggleButton^>(root->FindName(L"appBarToggleButton1"));
            appBarToggleButton2 = safe_cast<xaml_controls::AppBarToggleButton^>(root->FindName(L"appBarToggleButton2"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenCommandBar(cmdBar, OpenMethod::Touch);

        // Verify PrimaryCommands:
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton1, L"ApplicationViewStates", L"FullSize"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton1, L"InputModeStates", L"InputModeDefault"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarToggleButton1, L"ApplicationViewStates", L"FullSize"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarToggleButton1, L"InputModeStates", L"InputModeDefault"));

        // Verify SecondaryCommands:
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton2, L"ApplicationViewStates", L"OverflowWithToggleButtons"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton2, L"InputModeStates", L"TouchInputMode"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarToggleButton2, L"ApplicationViewStates", L"Overflow"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarToggleButton2, L"InputModeStates", L"TouchInputMode"));

        // Move AppBarToggleButton from Secondary to Primary commands:
        RunOnUIThread([&]()
        {
            ControlHelper::RemoveItem(cmdBar->SecondaryCommands, appBarToggleButton2);
            cmdBar->PrimaryCommands->Append(appBarToggleButton2);
        });
        TestServices::WindowHelper->WaitForIdle();

        // appBarButton2 should switch from "OverflowWithToggleButtons" to "Overflow"
        // (It is no longer sharing the overflow menu with any AppBarToggleButtons):
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton2, L"ApplicationViewStates", L"Overflow"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton2, L"InputModeStates", L"TouchInputMode"));

        // appBarToggleButton2 should no longer be in Overflow state or in Touch state:
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarToggleButton2, L"ApplicationViewStates", L"FullSize"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarToggleButton2, L"InputModeStates", L"InputModeDefault"));

        // Move AppBarButton and AppBarToggleButton from Primary to Secondary:
        RunOnUIThread([&]()
        {
            ControlHelper::RemoveItem(cmdBar->PrimaryCommands, appBarButton1);
            ControlHelper::RemoveItem(cmdBar->PrimaryCommands, appBarToggleButton1);
            cmdBar->SecondaryCommands->Append(appBarButton1);
            cmdBar->SecondaryCommands->Append(appBarToggleButton1);
        });
        TestServices::WindowHelper->WaitForIdle();

        // appBarButton2 should switch from "Overflow" to "OverflowWithToggleButtons":
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton2, L"ApplicationViewStates", L"OverflowWithToggleButtons"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton2, L"InputModeStates", L"TouchInputMode"));

        // appBarButton1 and appBarButton2 should switch to Overflow and Touch states:
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton1, L"ApplicationViewStates", L"OverflowWithToggleButtons"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton1, L"InputModeStates", L"TouchInputMode"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarToggleButton1, L"ApplicationViewStates", L"Overflow"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarToggleButton1, L"InputModeStates", L"TouchInputMode"));

        // Move AppBarButton from Secondary to Primary
        RunOnUIThread([&]()
        {
            ControlHelper::RemoveItem(cmdBar->SecondaryCommands, appBarButton2);
            cmdBar->PrimaryCommands->Append(appBarButton2);
        });
        TestServices::WindowHelper->WaitForIdle();

        // appBarButton2 should no longer be in Overflow state or in Touch state:
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton2, L"ApplicationViewStates", L"FullSize"));
        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(appBarButton2, L"InputModeStates", L"InputModeDefault"));

        CloseCommandBar(cmdBar);
    }

    void CommandBarIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedCommandBarWidth = 500;

        double const expectedCommandBarCompactClosedHeight = 48;
        double const expectedCommandBarCompactOpenHeight = 48;

        double const expectedCommandBarMinimalClosedHeight = 24;
        double const expectedCommandBarMinimalOpenHeight = 24;

        double const expectedCommandBarHiddenClosedHeight = 0;
        double const expectedCommandBarHiddenOpenHeight = 0;

        xaml_controls::CommandBar^ cmdBarCompactClosed = nullptr;
        xaml_controls::CommandBar^ cmdBarCompactOpen = nullptr;
        xaml_controls::CommandBar^ cmdBarMinimalClosed = nullptr;
        xaml_controls::CommandBar^ cmdBarMinimalOpen = nullptr;
        xaml_controls::CommandBar^ cmdBarHiddenClosed = nullptr;
        xaml_controls::CommandBar^ cmdBarHiddenOpen = nullptr;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <CommandBar x:Name="cmdBarCompactClosed" IsOpen="False" ClosedDisplayMode="Compact"/>
                        <CommandBar x:Name="cmdBarCompactOpen" IsOpen="True" ClosedDisplayMode="Compact"/>
                        <CommandBar x:Name="cmdBarMinimalClosed" IsOpen="False" ClosedDisplayMode="Minimal"/>
                        <CommandBar x:Name="cmdBarMinimalOpen" IsOpen="True" ClosedDisplayMode="Minimal"/>
                        <CommandBar x:Name="cmdBarHiddenClosed" IsOpen="False" ClosedDisplayMode="Hidden"/>
                        <CommandBar x:Name="cmdBarHiddenOpen" IsOpen="True" ClosedDisplayMode="Hidden"/>
                    </StackPanel>)"));

            cmdBarCompactClosed = safe_cast<xaml_controls::CommandBar^>(rootPanel->FindName(L"cmdBarCompactClosed"));
            cmdBarCompactOpen = safe_cast<xaml_controls::CommandBar^>(rootPanel->FindName(L"cmdBarCompactOpen"));
            cmdBarMinimalClosed = safe_cast<xaml_controls::CommandBar^>(rootPanel->FindName(L"cmdBarMinimalClosed"));
            cmdBarMinimalOpen = safe_cast<xaml_controls::CommandBar^>(rootPanel->FindName(L"cmdBarMinimalOpen"));
            cmdBarHiddenClosed = safe_cast<xaml_controls::CommandBar^>(rootPanel->FindName(L"cmdBarHiddenClosed"));
            cmdBarHiddenOpen = safe_cast<xaml_controls::CommandBar^>(rootPanel->FindName(L"cmdBarHiddenOpen"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedCommandBarWidth, cmdBarCompactClosed->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCommandBarCompactClosedHeight, cmdBarCompactClosed->ActualHeight);

            VERIFY_ARE_EQUAL(expectedCommandBarWidth, cmdBarCompactOpen->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCommandBarCompactOpenHeight, cmdBarCompactOpen->ActualHeight);

            VERIFY_ARE_EQUAL(expectedCommandBarWidth, cmdBarMinimalClosed->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCommandBarMinimalClosedHeight, cmdBarMinimalClosed->ActualHeight);

            VERIFY_ARE_EQUAL(expectedCommandBarWidth, cmdBarMinimalOpen->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCommandBarMinimalOpenHeight, cmdBarMinimalOpen->ActualHeight);

            VERIFY_ARE_EQUAL(expectedCommandBarWidth, cmdBarHiddenClosed->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCommandBarHiddenClosedHeight, cmdBarHiddenClosed->ActualHeight);

            VERIFY_ARE_EQUAL(expectedCommandBarWidth, cmdBarHiddenOpen->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCommandBarHiddenOpenHeight, cmdBarHiddenOpen->ActualHeight);
        });
    }

    void CommandBarIntegrationTests::ValidateDefaultLayoutPositionPropagates()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ btn1 = nullptr;
        xaml_controls::AppBarToggleButton^ btn2 = nullptr;
        xaml_controls::AppBarButton^ btn3 = nullptr;
        xaml_controls::AppBarToggleButton^ btn4 = nullptr;
        xaml_controls::AppBarButton^ btn5 = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto pageLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto appBarButtonLoadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, Loaded);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->DefaultLabelPosition = xaml_controls::CommandBarDefaultLabelPosition::Right;

            btn1 = ref new xaml_controls::AppBarButton();
            btn1->Label = L"btn1";
            cmdBar->PrimaryCommands->Append(btn1);

            btn2 = ref new xaml_controls::AppBarToggleButton();
            btn2->Label = L"btn2";
            cmdBar->PrimaryCommands->Append(btn2);

            btn3 = ref new xaml_controls::AppBarButton();
            btn3->Label = L"btn3";
            btn3->LabelPosition = xaml_controls::CommandBarLabelPosition::Collapsed;
            cmdBar->PrimaryCommands->Append(btn3);

            btn4 = ref new xaml_controls::AppBarToggleButton();
            btn4->Label = L"btn4";
            btn4->LabelPosition = xaml_controls::CommandBarLabelPosition::Collapsed;
            cmdBar->PrimaryCommands->Append(btn4);

            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = cmdBar;

            pageLoadedRegistration.Attach(page, [&]() { loadedEvent->Set(); });
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"DefaultLabelPosition initialized to Right.  Buttons without a LabelPosition set should be in the visual state LabelOnRight.");
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn1, L"ApplicationViewStates", L"LabelOnRight"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn2, L"ApplicationViewStates", L"LabelOnRight"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn3, L"ApplicationViewStates", L"LabelCollapsed"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn4, L"ApplicationViewStates", L"LabelCollapsed"));

            LOG_OUTPUT(L"Setting DefaultLabelPosition to Collapsed.  All buttons should be in the visual state LabelCollapsed.");
            cmdBar->DefaultLabelPosition = xaml_controls::CommandBarDefaultLabelPosition::Collapsed;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn1, L"ApplicationViewStates", L"LabelCollapsed"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn2, L"ApplicationViewStates", L"LabelCollapsed"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn3, L"ApplicationViewStates", L"LabelCollapsed"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn4, L"ApplicationViewStates", L"LabelCollapsed"));

            LOG_OUTPUT(L"Setting DefaultLabelPosition to Bottom.  Buttons without a LabelPosition set should be in the visual state Compact.");
            cmdBar->DefaultLabelPosition = xaml_controls::CommandBarDefaultLabelPosition::Bottom;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn1, L"ApplicationViewStates", L"Compact"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn2, L"ApplicationViewStates", L"Compact"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn3, L"ApplicationViewStates", L"LabelCollapsed"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn4, L"ApplicationViewStates", L"LabelCollapsed"));

            LOG_OUTPUT(L"Setting DefaultLabelPosition to Right.  Buttons without a LabelPosition set should be in the visual state LabelOnRight.");
            cmdBar->DefaultLabelPosition = xaml_controls::CommandBarDefaultLabelPosition::Right;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn1, L"ApplicationViewStates", L"LabelOnRight"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn2, L"ApplicationViewStates", L"LabelOnRight"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn3, L"ApplicationViewStates", L"LabelCollapsed"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn4, L"ApplicationViewStates", L"LabelCollapsed"));

            LOG_OUTPUT(L"Adding a new AppBarButton.  Its visual state should become LabelOnRight.");
            btn5 = ref new xaml_controls::AppBarButton();
            btn5->Label = L"btn5";
            cmdBar->PrimaryCommands->Append(btn5);

            appBarButtonLoadedRegistration.Attach(btn5, [&]() { loadedEvent->Set(); });
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(btn5, L"ApplicationViewStates", L"LabelOnRight"));
        });
    }

    void CommandBarIntegrationTests::ValidateOverflowButtonHidesWhenAppropriateWithNoAppBarButtons()
    {
        ValidateOverflowButtonHidesWhenAppropriate(false /* addPrimary */, false /* addSecondary */);
    }

    void CommandBarIntegrationTests::ValidateOverflowButtonHidesWhenAppropriateWithPrimaryAppBarButtons()
    {
        ValidateOverflowButtonHidesWhenAppropriate(true /* addPrimary */, false /* addSecondary */);
    }

    void CommandBarIntegrationTests::ValidateOverflowButtonHidesWhenAppropriateWithSecondaryAppBarButtons()
    {
        ValidateOverflowButtonHidesWhenAppropriate(false /* addPrimary */, true /* addSecondary */);
    }

    void CommandBarIntegrationTests::ValidateOverflowButtonHidesWhenAppropriateWithPrimaryAndSecondaryAppBarButtons()
    {
        ValidateOverflowButtonHidesWhenAppropriate(true /* addPrimary */, true /* addSecondary */);
    }

    void CommandBarIntegrationTests::ValidateOverflowButtonHidesWhenAppropriate(
        bool addPrimary,
        bool addSecondary)
    {
        TestCleanupWrapper cleanup;

        for (int closedDisplayModeValue = 0; closedDisplayModeValue < 3; closedDisplayModeValue++)
        {
            for (int defaultLabelPositionValue = 0; defaultLabelPositionValue < 3; defaultLabelPositionValue++)
            {
                for (int overflowButtonVisibilityValue = 0; overflowButtonVisibilityValue < 3; overflowButtonVisibilityValue++)
                {
                    xaml_controls::AppBarClosedDisplayMode closedDisplayMode = static_cast<xaml_controls::AppBarClosedDisplayMode>(closedDisplayModeValue);
                    xaml_controls::CommandBarDefaultLabelPosition defaultLabelPosition = static_cast<xaml_controls::CommandBarDefaultLabelPosition>(defaultLabelPositionValue);
                    xaml_controls::CommandBarOverflowButtonVisibility overflowButtonVisibility = static_cast<xaml_controls::CommandBarOverflowButtonVisibility>(overflowButtonVisibilityValue);

                    LOG_OUTPUT(
                        L"Testing the overflow button's visibility with %s, %s, ClosedDisplayMode = %s, DefaultLabelPosition = %s, and OverflowButtonVisibility = %s.",
                        addPrimary ? L"a primary button" : L"no primary buttons",
                        addSecondary ? L"a secondary button" : L"no secondary buttons",
                        closedDisplayMode == xaml_controls::AppBarClosedDisplayMode::Compact ? L"Compact" : (closedDisplayMode == xaml_controls::AppBarClosedDisplayMode::Minimal ? L"Minimal" : L"Hidden"),
                        defaultLabelPosition == xaml_controls::CommandBarDefaultLabelPosition::Bottom ? L"Bottom" : (defaultLabelPosition == xaml_controls::CommandBarDefaultLabelPosition::Right ? L"Right" : L"Collapsed"),
                        overflowButtonVisibility == xaml_controls::CommandBarOverflowButtonVisibility::Auto ? L"Auto" : (overflowButtonVisibility == xaml_controls::CommandBarOverflowButtonVisibility::Visible ? L"Visible" : L"Collapsed"));

                    // We expect the overflow button to be visible if we tell it to be visible through OverflowButtonVisibility,
                    // or if OverflowButtonVisibility is Auto and either there are secondary items or there is at least one primary item
                    // with a bottom-aligned label, or if ClosedDisplayMode is something other than Compact.
                    xaml::Visibility expectedOverflowButtonVisibility =
                        overflowButtonVisibility == xaml_controls::CommandBarOverflowButtonVisibility::Visible ||
                        (overflowButtonVisibility == xaml_controls::CommandBarOverflowButtonVisibility::Auto &&
                            (addSecondary ||
                            (addPrimary && defaultLabelPosition == xaml_controls::CommandBarDefaultLabelPosition::Bottom) ||
                            closedDisplayMode != xaml_controls::AppBarClosedDisplayMode::Compact)) ?
                        xaml::Visibility::Visible :
                        xaml::Visibility::Collapsed;

                    LOG_OUTPUT(
                        L"We expect the overflow button to be %s.",
                        expectedOverflowButtonVisibility == xaml::Visibility::Visible ? L"visible" : L"collapsed");

                    ValidateOverflowButtonState(
                        addPrimary,
                        addSecondary,
                        closedDisplayMode,
                        defaultLabelPosition,
                        overflowButtonVisibility,
                        expectedOverflowButtonVisibility);
                }
            }
        }
    }

    void CommandBarIntegrationTests::ValidateOverflowButtonState(
        bool addPrimary,
        bool addSecondary,
        xaml_controls::AppBarClosedDisplayMode closedDisplayMode,
        xaml_controls::CommandBarDefaultLabelPosition defaultLabelPosition,
        xaml_controls::CommandBarOverflowButtonVisibility overflowButtonVisibility,
        xaml::Visibility expectedOverflowButtonVisibility)
    {
        xaml_controls::CommandBar^ cmdBar = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            auto rootGrid = ref new xaml_controls::Grid();

            loadedRegistration.Attach(rootGrid, [&]() { loadedEvent->Set(); });

            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->ClosedDisplayMode = closedDisplayMode;
            cmdBar->DefaultLabelPosition = defaultLabelPosition;
            cmdBar->OverflowButtonVisibility = overflowButtonVisibility;

            if (addPrimary)
            {
                auto button = ref new xaml_controls::AppBarButton();
                button->Label = "button";
                cmdBar->PrimaryCommands->Append(button);
            }

            if (addSecondary)
            {
                auto button = ref new xaml_controls::AppBarButton();
                button->Label = "button";
                cmdBar->SecondaryCommands->Append(button);
            }

            rootGrid->Children->Append(cmdBar);
            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedOverflowButtonVisibility, TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton")->Visibility);
        });
    }

    // Validate the dynamic overflow behavior with on and off IsDynamicOverflowEnabled property
    void CommandBarIntegrationTests::ValidateDynamicOverflowOnOff()
    {
        TestCleanupWrapper cleanup;

        const unsigned int numButtonsToAddExtraToPrimary = 5;
        const unsigned int numButtonsToAddToSecondary = 3;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);
        ValidateDynamicOverflowWorker(cmdBar, true /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        RunOnUIThread([&]()
        {
            cmdBar->IsDynamicOverflowEnabled = false;
            LOG_OUTPUT(L"DynamicOverflow is disabled!");
        });
        TestServices::WindowHelper->WaitForIdle();

        ValidateDynamicOverflowWorker(cmdBar, false /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        auto dynamicOverflowItemsChangingEvent = std::make_shared<Event>();
        auto dynamicOverflowItemsChangingRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, DynamicOverflowItemsChanging);

        RunOnUIThread([&]()
        {
            dynamicOverflowItemsChangingRegistration.Attach(cmdBar, [&]()
            {
                dynamicOverflowItemsChangingEvent->Set();
            });

            cmdBar->IsDynamicOverflowEnabled = true;
            LOG_OUTPUT(L"DynamicOverflow is enabled!");
        });
        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        ValidateDynamicOverflowWorker(cmdBar, true /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowByChangingWindowsSizeOverride()
    {
        TestCleanupWrapper cleanup;

        unsigned int numButtonsToAddExtraToPrimary = 0;
        unsigned int numButtonsToAddToSecondary = 3;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);
        ValidateDynamicOverflowWorker(cmdBar, false /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(300, 600));

        numButtonsToAddExtraToPrimary = 2;
        page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);
        ValidateDynamicOverflowWorker(cmdBar, true /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowAddRemovePrimaryItems()
    {
        TestCleanupWrapper cleanup;

        const unsigned int numButtonsToAddExtraToPrimary = 5;
        const unsigned int numButtonsToAddToSecondary = 3;

        xaml_controls::AppBarButton^ addedButton1 = nullptr;
        xaml_controls::AppBarButton^ addedButton2 = nullptr;
        xaml_controls::AppBarButton^ addedButton3 = nullptr;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);
        ValidateDynamicOverflowWorker(cmdBar, true /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        RunOnUIThread([&]()
        {
            unsigned int primaryCommandsCount = cmdBar->PrimaryCommands->Size;

            addedButton1 = ref new xaml_controls::AppBarButton();
            cmdBar->PrimaryCommands->Append(addedButton1);

            VERIFY_IS_TRUE(addedButton1 == cmdBar->PrimaryCommands->GetAt(primaryCommandsCount));

            addedButton2 = ref new xaml_controls::AppBarButton();
            cmdBar->PrimaryCommands->InsertAt(0, addedButton2);

            VERIFY_IS_TRUE(addedButton2 == cmdBar->PrimaryCommands->GetAt(0));

            addedButton3 = ref new xaml_controls::AppBarButton();
            cmdBar->SecondaryCommands->InsertAt(0, addedButton3);

            VERIFY_IS_TRUE(addedButton3 == cmdBar->SecondaryCommands->GetAt(0));
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenCommandBar(cmdBar, OpenMethod::Programmatic);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(addedButton1->IsInOverflow);
            VERIFY_IS_FALSE(addedButton2->IsInOverflow);
            VERIFY_IS_TRUE(addedButton3->IsInOverflow);

            cmdBar->PrimaryCommands->RemoveAt(0);
            VERIFY_IS_FALSE(addedButton2 == cmdBar->PrimaryCommands->GetAt(0));

            cmdBar->SecondaryCommands->RemoveAt(0);
            VERIFY_IS_FALSE(addedButton3 == cmdBar->PrimaryCommands->GetAt(0));
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseCommandBar(cmdBar);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowAppBarSeparator()
    {
        TestCleanupWrapper cleanup;

        unsigned int numButtonsToAddExtraToPrimary = 0;
        unsigned int numButtonsToAddToSecondary = 3;

        xaml_controls::AppBarSeparator^ addedSeparator1 = nullptr;
        xaml_controls::AppBarSeparator^ addedSeparator2 = nullptr;
        xaml_controls::AppBarButton^ addedButton1 = nullptr;
        xaml_controls::AppBarButton^ addedButton2 = nullptr;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        RunOnUIThread([&]()
        {
            unsigned int primaryCommandsCount = cmdBar->PrimaryCommands->Size;

            addedSeparator1 = ref new xaml_controls::AppBarSeparator();
            cmdBar->PrimaryCommands->Append(addedSeparator1);

            VERIFY_IS_TRUE(addedSeparator1 == cmdBar->PrimaryCommands->GetAt(primaryCommandsCount));

            addedSeparator2 = ref new xaml_controls::AppBarSeparator();
            cmdBar->PrimaryCommands->InsertAt(0, addedSeparator2);

            VERIFY_IS_TRUE(addedSeparator2 == cmdBar->PrimaryCommands->GetAt(0));

            addedButton1 = ref new xaml_controls::AppBarButton();
            cmdBar->PrimaryCommands->InsertAt(0, addedButton1);

            VERIFY_IS_TRUE(addedButton1 == cmdBar->PrimaryCommands->GetAt(0));

            addedButton2 = ref new xaml_controls::AppBarButton();
            cmdBar->PrimaryCommands->InsertAt(0, addedButton2);

            VERIFY_IS_TRUE(addedButton2 == cmdBar->PrimaryCommands->GetAt(0));
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenCommandBar(cmdBar, OpenMethod::Programmatic);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(addedSeparator1->IsInOverflow);
            VERIFY_IS_FALSE(addedSeparator2->IsInOverflow);
            VERIFY_IS_FALSE(addedButton1->IsInOverflow);
            VERIFY_IS_FALSE(addedButton2->IsInOverflow);

            cmdBar->PrimaryCommands->RemoveAt(0);
            cmdBar->PrimaryCommands->RemoveAt(0);
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseCommandBar(cmdBar);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(addedSeparator1->IsInOverflow);
            VERIFY_IS_FALSE(addedSeparator2->IsInOverflow);
        });
    }

    void CommandBarIntegrationTests::ValidateFireDynamicOverflowItemsChangingEvent()
    {
        TestCleanupWrapper cleanup;

        unsigned int numButtonsToAddExtraToPrimary = 2;
        unsigned int numButtonsToAddToSecondary = 3;

        xaml_controls::AppBarButton^ addedButton1 = nullptr;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        ValidateDynamicOverflowItemsChangingEventWorker(cmdBar, true);
        ValidateDynamicOverflowItemsChangingEventWorker(cmdBar, false);
        ValidateDynamicOverflowItemsChangingEventWorker(cmdBar, false);
        ValidateDynamicOverflowItemsChangingEventWorker(cmdBar, true);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowWithChangingOrientation()
    {
        TestCleanupWrapper cleanup;

        // The test sets up its tree to use a BottomAppBar, so make sure to call SetWindowSizeOverride() before setting up the tree
        // because it doesn't raise WindowSizeChanged and the AppBarService depends on that to correctly size Top/BottomAppBars.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 480));

        unsigned int numButtonsToAddExtraToPrimary = 0;
        unsigned int numButtonsToAddToSecondary = 3;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        ValidateDynamicOverflowWorker(cmdBar, false /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(480, 800));

        numButtonsToAddExtraToPrimary = 5;
        page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);

        OpenCommandBar(cmdBar, OpenMethod::Programmatic);

        CloseCommandBar(cmdBar);

        ValidateDynamicOverflowWorker(cmdBar, true /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowOrderBasic()
    {
        TestCleanupWrapper cleanup;

        unsigned int numButtonsToAddExtraToPrimary = 2;
        unsigned int numButtonsToAddToSecondary = 3;

        xaml_controls::AppBarButton^ addedButton1 = nullptr;
        xaml_controls::AppBarSeparator^ addedSeparator1 = nullptr;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary, true /*isSetOrder*/);
        ValidateDynamicOverflowWorker(cmdBar, true /* isPrimaryCommandMovedToOverflow */, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary, true /*isSetOrder*/);

        RunOnUIThread([&]()
        {
            addedButton1 = ref new xaml_controls::AppBarButton();
            addedButton1->DynamicOverflowOrder = 1;
            cmdBar->PrimaryCommands->InsertAt(0, addedButton1);
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenCommandBar(cmdBar, OpenMethod::Programmatic);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(addedButton1->IsInOverflow);
        });
        TestServices::WindowHelper->WaitForIdle();

        CloseCommandBar(cmdBar);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowOrderTestCases()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ root = nullptr;
        xaml_controls::CommandBar^ cmdBar1 = nullptr;
        xaml_controls::CommandBar^ cmdBar2 = nullptr;
        xaml_controls::CommandBar^ cmdBar3 = nullptr;
        xaml_controls::CommandBar^ cmdBar4 = nullptr;
        xaml_controls::CommandBar^ cmdBar5 = nullptr;
        xaml_controls::CommandBar^ cmdBar6 = nullptr;

        RunOnUIThread([&]()
        {
            root = ref new xaml_controls::StackPanel();

            cmdBar1 = CreateCommandBarWithPrimaryCommandOrderSet(DynamicOverflowOrderTest::OrderTestForAlternativeValue);
            cmdBar2 = CreateCommandBarWithPrimaryCommandOrderSet(DynamicOverflowOrderTest::OrderTestForAllSameValue);
            cmdBar3 = CreateCommandBarWithPrimaryCommandOrderSet(DynamicOverflowOrderTest::OrderTestForTwoPairedValue);
            cmdBar4 = CreateCommandBarWithPrimaryCommandOrderSet(DynamicOverflowOrderTest::OrderTestForFallbackDefault);
            cmdBar5 = CreateCommandBarWithPrimaryCommandOrderSet(DynamicOverflowOrderTest::OrderTestForMovingPriorSeparator);
            cmdBar6 = CreateCommandBarWithPrimaryCommandOrderSet(DynamicOverflowOrderTest::OrderTestForMovingPosteriorSeparator);

            root->Children->Append(cmdBar1);
            root->Children->Append(cmdBar2);
            root->Children->Append(cmdBar3);
            root->Children->Append(cmdBar4);
            root->Children->Append(cmdBar5);
            root->Children->Append(cmdBar6);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Test for alternative order - {1,2,1,2,1,2,1,2,1,2}
        ValidateDynamicOverflowOrderWorker(cmdBar1, DynamicOverflowOrderTest::OrderTestForAlternativeValue);

        // Test for all same order - {1,1,1,1,1,1,1,1,1,1}
        ValidateDynamicOverflowOrderWorker(cmdBar2, DynamicOverflowOrderTest::OrderTestForAllSameValue);

        // Test for paired order group - {1,2,3,4,5,1,2,3,4,5}
        ValidateDynamicOverflowOrderWorker(cmdBar3, DynamicOverflowOrderTest::OrderTestForTwoPairedValue);

        // Test for order set and default rightmost dynamic overflow - {1, 1, 2, 2, 0, 0, 0, 0, 0, 0}
        ValidateDynamicOverflowOrderWorker(cmdBar4, DynamicOverflowOrderTest::OrderTestForFallbackDefault);

        // Test for order set with moving the separator together - {|, 1, |, 2, 3, 4, 5, 1, 2, 3, 4, 5}
        ValidateDynamicOverflowOrderWorker(cmdBar5, DynamicOverflowOrderTest::OrderTestForMovingPriorSeparator);

        // Test for order set and default rightmost dynamic overflow with moving the separator together  - {0, 0, 0, 0, 0, 0, 0, 0, 0, |, 1, |}
        ValidateDynamicOverflowOrderWorker(cmdBar6, DynamicOverflowOrderTest::OrderTestForMovingPosteriorSeparator);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowWithContentControl()
    {
        TestCleanupWrapper cleanup;

        const unsigned int numButtonsToAddExtraToPrimary = 5;
        const unsigned int numButtonsToAddToSecondary = 3;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = SetupDynamicOverflowTest(&cmdBar, numButtonsToAddExtraToPrimary, numButtonsToAddToSecondary);
        xaml_controls::TextBox^ textBox = nullptr;

        RunOnUIThread([&]()
        {
            textBox = ref new xaml_controls::TextBox();
            textBox->Width = 100;

            cmdBar->Content = textBox;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto primaryItemsControl = safe_cast<xaml_controls::ItemsControl^>(TreeHelper::GetVisualChildByName(cmdBar, L"PrimaryItemsControl"));

            wf::Rect primaryItemsControlBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(primaryItemsControl));
            wf::Rect textBoxBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(textBox));

            LOG_OUTPUT(L"TextBox bounds left=%f top=%f width=%f height=%f", textBoxBounds.Left, textBoxBounds.Top, textBoxBounds.Width, textBoxBounds.Height);
            LOG_OUTPUT(L"Primary ItemsControl bounds left=%f top=%f width=%f height=%f", primaryItemsControlBounds.Left, primaryItemsControlBounds.Top, primaryItemsControlBounds.Width, primaryItemsControlBounds.Height);

            // Validate the TextBox doesn't occlude with the primary ItemsControl by applying the dynamic overflow mechanism
            VERIFY_IS_LESS_THAN(textBoxBounds.Right, primaryItemsControlBounds.X);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cmdBar->IsDynamicOverflowEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto primaryItemsControl = safe_cast<xaml_controls::ItemsControl^>(TreeHelper::GetVisualChildByName(cmdBar, L"PrimaryItemsControl"));

            wf::Rect primaryItemsControlBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(primaryItemsControl));
            wf::Rect textBoxBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(textBox));

            LOG_OUTPUT(L"TextBox bounds left=%f top=%f width=%f height=%f", textBoxBounds.Left, textBoxBounds.Top, textBoxBounds.Width, textBoxBounds.Height);
            LOG_OUTPUT(L"Primary ItemsControl bounds left=%f top=%f width=%f height=%f", primaryItemsControlBounds.Left, primaryItemsControlBounds.Top, primaryItemsControlBounds.Width, primaryItemsControlBounds.Height);

            // Validate the TextBox is occluded with the primary ItemsControl when the primary ItemsControl has lots primary items
            // and disabled dynamic overflow mechanism
            VERIFY_IS_GREATER_THAN(textBoxBounds.Right, primaryItemsControlBounds.X);
        });
        TestServices::WindowHelper->WaitForIdle();

        EmptyPageContent(page);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowWorker(
        xaml_controls::CommandBar^ cmdBar,
        bool isPrimaryCommandMovedToOverflow,
        unsigned int numButtonsToAddExtraToPrimary,
        unsigned int numButtonsToAddToSecondary,
        bool isSetOrder)
    {
        unsigned int primaryItemsCount = 0;

        RunOnUIThread([&]()
        {
            auto primaryItemsControl = safe_cast<xaml_controls::ItemsControl^>(TreeHelper::GetVisualChildByName(cmdBar, L"PrimaryItemsControl"));
            auto primaryItems = primaryItemsControl->Items;

            primaryItemsCount = primaryItems->Size;

            LOG_OUTPUT(L"Number of buttons in CommandBar.PrimaryCommands: %d", cmdBar->PrimaryCommands->Size);
            LOG_OUTPUT(L"Number of buttons in CommandBar.SecondaryCommands: %d", cmdBar->SecondaryCommands->Size);
            LOG_OUTPUT(L"Number of buttons in CommandBar.PrimaryItemsControl Count: %d", primaryItemsCount);
            LOG_OUTPUT(L"PrimaryItemsControl ActualWidth = %f", primaryItemsControl->ActualWidth);

            if (isPrimaryCommandMovedToOverflow)
            {
                auto button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(0));
                auto moreButton = GetMoreButton(cmdBar);
                unsigned int numButtonsToStayOnPrimary = static_cast<unsigned int>((cmdBar->ActualWidth - moreButton->ActualWidth) / button->ActualWidth);

                VERIFY_IS_TRUE(primaryItemsCount == numButtonsToStayOnPrimary);
                VERIFY_IS_TRUE(cmdBar->PrimaryCommands->Size == primaryItemsCount + numButtonsToAddExtraToPrimary);
            }
            else
            {
                VERIFY_IS_TRUE(cmdBar->PrimaryCommands->Size == primaryItemsCount);
            }

            bool areAllPrimaryItemsCompact = true;
            for (unsigned int i = 0; i < primaryItems->Size; i++)
            {
                auto button = safe_cast<xaml_controls::AppBarButton^>(primaryItems->GetAt(i));
                areAllPrimaryItemsCompact = areAllPrimaryItemsCompact && safe_cast<xaml_controls::AppBarButton^>(primaryItems->GetAt(i))->IsCompact;
            }
            VERIFY_IS_TRUE(areAllPrimaryItemsCompact);
        });

        OpenCommandBar(cmdBar, OpenMethod::Programmatic);

        RunOnUIThread([&]()
        {
            unsigned int numButtonsPrimaryCommandsInOverflow = cmdBar->PrimaryCommands->Size - primaryItemsCount;

            auto overflowContentRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar));
            auto secondaryItemsControl = safe_cast<xaml_controls::ItemsControl^>(TreeHelper::GetVisualChildByName(overflowContentRoot, L"SecondaryItemsControl"));
            auto secondaryItems = secondaryItemsControl->Items;

            LOG_OUTPUT(L"Number of primary buttons in current Primary items : %d", primaryItemsCount);
            LOG_OUTPUT(L"Number of buttons in current SecondaryCommands items : %d", secondaryItems->Size);
            LOG_OUTPUT(L"Number of primary buttons in overflow: %d", numButtonsPrimaryCommandsInOverflow);

            VERIFY_IS_TRUE(isPrimaryCommandMovedToOverflow ? numButtonsPrimaryCommandsInOverflow == numButtonsToAddExtraToPrimary : numButtonsPrimaryCommandsInOverflow == 0);
            VERIFY_IS_TRUE(secondaryItems->Size == numButtonsPrimaryCommandsInOverflow + numButtonsToAddToSecondary + isPrimaryCommandMovedToOverflow ? 1 : 0);

            for (unsigned int i = 0; i < secondaryItems->Size; i++)
            {
                if (i < numButtonsPrimaryCommandsInOverflow)
                {
                    auto button = safe_cast<xaml_controls::AppBarButton^>(secondaryItems->GetAt(i));
                    VERIFY_IS_TRUE(ControlHelper::IsInVisualState(button, L"ApplicationViewStates", L"Overflow"));
                }
                else if (isPrimaryCommandMovedToOverflow && i == numButtonsPrimaryCommandsInOverflow)
                {
                    auto overflowSeparator = safe_cast<xaml_controls::AppBarSeparator^>(secondaryItems->GetAt(i));
                    VERIFY_IS_TRUE(ControlHelper::IsInVisualState(overflowSeparator, L"ApplicationViewStates", L"Overflow"));
                }
                else
                {
                    auto button = safe_cast<xaml_controls::AppBarButton^>(secondaryItems->GetAt(i));
                    VERIFY_IS_TRUE(ControlHelper::IsInVisualState(button, L"ApplicationViewStates", L"Overflow"));
                }
            }

            if (isSetOrder)
            {
                int maxOrderInOverflow = 0;

                for (unsigned int i = 0; i < numButtonsPrimaryCommandsInOverflow; i++)
                {
                    auto button = safe_cast<xaml_controls::AppBarButton^>(secondaryItems->GetAt(i));
                    if (button->DynamicOverflowOrder > maxOrderInOverflow)
                    {
                        maxOrderInOverflow = button->DynamicOverflowOrder;
                    }
                }

                auto primaryItemsControl = safe_cast<xaml_controls::ItemsControl^>(TreeHelper::GetVisualChildByName(cmdBar, L"PrimaryItemsControl"));
                auto primaryItems = primaryItemsControl->Items;

                for (unsigned int i = 0; i < primaryItems->Size; i++)
                {
                    auto button = safe_cast<xaml_controls::AppBarButton^>(primaryItems->GetAt(i));
                    if (button->DynamicOverflowOrder != 0)
                    {
                        VERIFY_IS_TRUE(button->DynamicOverflowOrder > maxOrderInOverflow);
                    }
                }
            }
        });

        CloseCommandBar(cmdBar);
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowItemsChangingEventWorker(xaml_controls::CommandBar^ cmdBar, bool isAdding)
    {
        auto dynamicOverflowItemsChangingEvent = std::make_shared<Event>();
        auto dynamicOverflowItemsChangingRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, DynamicOverflowItemsChanging);

        dynamicOverflowItemsChangingRegistration.Attach(
            cmdBar, ref new wf::TypedEventHandler<xaml_controls::CommandBar^, xaml_controls::DynamicOverflowItemsChangingEventArgs^>(
                [&](Platform::Object^ sender, xaml_controls::DynamicOverflowItemsChangingEventArgs^ args)
        {
            if (args->Action == xaml_controls::CommandBarDynamicOverflowAction::AddingToOverflow)
            {
                VERIFY_IS_TRUE(isAdding);
            }
            else
            {
                VERIFY_IS_FALSE(isAdding);
            }
            dynamicOverflowItemsChangingEvent->Set();
        }));


        RunOnUIThread([&]()
        {
            if (isAdding)
            {
                auto addedButton1 = ref new xaml_controls::AppBarButton();
                cmdBar->PrimaryCommands->Append(addedButton1);
            }
            else
            {
                cmdBar->PrimaryCommands->RemoveAt(0);
            }
        });
        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowOrderWorker(xaml_controls::CommandBar^ cmdBar, DynamicOverflowOrderTest orderTestCase)
    {
        OpenCommandBar(cmdBar, OpenMethod::Programmatic);

        RunOnUIThread([&]()
        {
            auto primaryItemsControl = safe_cast<xaml_controls::ItemsControl^>(TreeHelper::GetVisualChildByName(cmdBar, L"PrimaryItemsControl"));
            auto primaryItems = primaryItemsControl->Items;
            unsigned int primaryItemsCount = primaryItems->Size;

            auto overflowContentRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"OverflowContentRoot", cmdBar));
            auto secondaryItemsControl = safe_cast<xaml_controls::ItemsControl^>(TreeHelper::GetVisualChildByName(overflowContentRoot, L"SecondaryItemsControl"));
            auto secondaryItems = secondaryItemsControl->Items;
            unsigned int secondaryCount = secondaryItems->Size;

            LOG_OUTPUT(L"Primary items counts : %d", primaryItemsCount);
            LOG_OUTPUT(L"Secondary items counts  : %d", secondaryCount);

            switch (orderTestCase)
            {
                case DynamicOverflowOrderTest::OrderTestForAlternativeValue: // Test for alternative order - {1,2,1,2,1,2,1,2,1,2}
                {
                    for (int i = 0; i < 10; i++)
                    {
                        if (i % 2 == 0)
                        {
                            auto button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(i));
                            VERIFY_IS_TRUE(button->IsInOverflow);
                        }
                    }
                    break;
                }
                case DynamicOverflowOrderTest::OrderTestForAllSameValue: // Test for all same order - {1,1,1,1,1,1,1,1,1,1}
                {
                    for (int i = 0; i < 10; i++)
                    {
                        auto button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(i));
                        VERIFY_IS_TRUE(button->IsInOverflow);
                    }
                    break;
                }
                case DynamicOverflowOrderTest::OrderTestForTwoPairedValue: // Test for paired order group - {1,2,3,4,5,1,2,3,4,5}
                {
                    VERIFY_IS_TRUE(primaryItemsCount == 4);
                    VERIFY_IS_TRUE(secondaryCount == 6);

                    auto button1 = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(0));
                    auto button8 = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(7));
                    VERIFY_IS_TRUE(button1->IsInOverflow);
                    VERIFY_IS_TRUE(button1->IsInOverflow);
                    break;
                }
                case DynamicOverflowOrderTest::OrderTestForFallbackDefault: // Test for order set and default rightmost dynamic overflow - {1,1,2,2,0,0,0,0,0,0}
                {
                    VERIFY_IS_TRUE(primaryItemsCount == 5);
                    VERIFY_IS_TRUE(secondaryCount == 5);

                    auto button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(0));
                    VERIFY_IS_TRUE(button->IsInOverflow);
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(1));
                    VERIFY_IS_TRUE(button->IsInOverflow);
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(2));
                    VERIFY_IS_TRUE(button->IsInOverflow);
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(3));
                    VERIFY_IS_TRUE(button->IsInOverflow);
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(9));
                    VERIFY_IS_TRUE(button->IsInOverflow);
                    break;
                }
                case DynamicOverflowOrderTest::OrderTestForMovingPriorSeparator: // Test for order set with moving the separator together - {|, 1, |, 2, 3, 4, 5, 1, 2, 3, 4, 5}
                {
                    auto separator = safe_cast<xaml_controls::AppBarSeparator^>(cmdBar->PrimaryCommands->GetAt(0));
                    VERIFY_IS_FALSE(separator->IsInOverflow);
                    auto button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(1));
                    VERIFY_IS_TRUE(button->IsInOverflow);
                    separator = safe_cast<xaml_controls::AppBarSeparator^>(cmdBar->PrimaryCommands->GetAt(2));
                    VERIFY_IS_FALSE(separator->IsInOverflow);
                    break;
                }
                case DynamicOverflowOrderTest::OrderTestForMovingPosteriorSeparator: // Test for order set and default rightmost dynamic overflow with moving the separator together  - {0, 0, 0, 0, 0, 0, 0, 0, 0, |, 1, |}
                {
                    auto separator = safe_cast<xaml_controls::AppBarSeparator^>(cmdBar->PrimaryCommands->GetAt(9));
                    VERIFY_IS_FALSE(separator->IsInOverflow);
                    auto button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(10));
                    VERIFY_IS_TRUE(button->IsInOverflow);
                    separator = safe_cast<xaml_controls::AppBarSeparator^>(cmdBar->PrimaryCommands->GetAt(11));
                    VERIFY_IS_FALSE(separator->IsInOverflow);
                    break;
                }
            }

        });
        TestServices::WindowHelper->WaitForIdle();

        CloseCommandBar(cmdBar);
    }

    void CommandBarIntegrationTests::ValidateVisualStateUpdatesWhenDynamicOverflowCausesItemsToMove()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::ItemsControl^ primaryItemsControl = nullptr;

        bool expectItemsAdded = false;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->Width = 600;

            auto appBarButton = ref new xaml_controls::AppBarButton();
            appBarButton->Label = L"Button";
            appBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
            cmdBar->PrimaryCommands->Append(appBarButton);

            auto button = ref new xaml_controls::Button();
            button->Content = L"Content Button";
            button->Width = 396;
            cmdBar->Content = button;

            auto grid = ref new xaml_controls::Grid();
            grid->Children->Append(cmdBar);
            loadedRegistration.Attach(grid, [&] { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = grid;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            primaryItemsControl = safe_cast<xaml_controls::ItemsControl^>(TreeHelper::GetVisualChildByName(cmdBar, L"PrimaryItemsControl"));
        });

        auto dynamicOverflowItemsChangingEvent = std::make_shared<Event>();
        auto dynamicOverflowItemsChangingRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, DynamicOverflowItemsChanging);

        dynamicOverflowItemsChangingRegistration.Attach(cmdBar,
            ref new wf::TypedEventHandler<xaml_controls::CommandBar^, xaml_controls::DynamicOverflowItemsChangingEventArgs^>(
            [&](xaml_controls::CommandBar^ sender, xaml_controls::DynamicOverflowItemsChangingEventArgs^ args)
            {
                if (expectItemsAdded)
                {
                    VERIFY_ARE_EQUAL(xaml_controls::CommandBarDynamicOverflowAction::AddingToOverflow, args->Action);
                }
                else
                {
                    VERIFY_ARE_EQUAL(xaml_controls::CommandBarDynamicOverflowAction::RemovingFromOverflow, args->Action);
                }

                dynamicOverflowItemsChangingEvent->Set();
            }));

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Change the width of the CommandBar to be 400.  The AppBarButton should be moved to the overflow.");
            expectItemsAdded = true;
            cmdBar->Width = 400;
        });

        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The primary items control should now be collapsed since there are no AppBarButtons left in it.");
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, primaryItemsControl->Visibility);
        });

        LOG_OUTPUT(L"Now open and close the CommandBar.");
        OpenCommandBar(cmdBar, OpenMethod::Programmatic);
        CloseCommandBar(cmdBar);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The primary items control should still be collapsed.");
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, primaryItemsControl->Visibility);

            LOG_OUTPUT(L"Change the width of the CommandBar back to 600.  The AppBarButton should be moved back from the overflow.");
            expectItemsAdded = false;
            cmdBar->Width = 600;
        });

        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The primary items control should now be visible since the AppBarButton is back in it.");
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, primaryItemsControl->Visibility);
        });
    }

    void CommandBarIntegrationTests::ValidateDynamicOverflowWithCustomAppBarButton()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            // Validate the dynamic overflow behaviors with the CustomAppBarButton that is implemented with the legacy
            // ICommandBarElement that doesn't have a property for setting DynamicOrder
            cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                                xmlns:local="using:Tests.External.Controls.CustomTypes">
                        <CommandBar.Resources>
                            <Style TargetType="local:CustomAppBarButton">
                                <Setter Property="Margin" Value="12,0"/>
                            </Style>
                        </CommandBar.Resources>
                        <local:CustomAppBarButton Content="First"/>
                        <local:CustomAppBarButton Content="Second"/>
                        <local:CustomAppBarButton Content="Third"/>
                        <local:CustomAppBarButton Content="Fourth"/>
                        <local:CustomAppBarButton Content="Fifth"/>
                        <local:CustomAppBarButton Content="Sixth"/>
                        <local:CustomAppBarButton Content="Seventh"/>
                        <local:CustomAppBarButton Content="Eighth"/>
                        <local:CustomAppBarButton Content="Ninth"/>
                        <local:CustomAppBarButton Content="Tenth"/>
                    </CommandBar>)"));

            auto rootGrid = ref new xaml_controls::Grid();
            rootGrid->Children->Append(cmdBar);
            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto dynamicOverflowItemsChangingEvent = std::make_shared<Event>();
        auto dynamicOverflowItemsChangingRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, DynamicOverflowItemsChanging);

        dynamicOverflowItemsChangingRegistration.Attach(cmdBar,
            ref new wf::TypedEventHandler<xaml_controls::CommandBar^, xaml_controls::DynamicOverflowItemsChangingEventArgs^>(
            [&](xaml_controls::CommandBar^ sender, xaml_controls::DynamicOverflowItemsChangingEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(xaml_controls::CommandBarDynamicOverflowAction::AddingToOverflow, args->Action);
            dynamicOverflowItemsChangingEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Set CommandBar width = 200.  The CustomAppBarButton should be moved to the overflow.");
            cmdBar->Width = 200;
        });
        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::ValidatePrimaryButtonWidthAtRightDefaultLabelPosition()
    {
        TestCleanupWrapper cleanup;

        // This testvalidate the primary buttons width that is applying
        // the dynamic overflow and the overflow styles at the Right DefaultLabelPosition.

        xaml_controls::CommandBar^ cmdBar = nullptr;

        double button1Width = 0;
        double button2Width = 0;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->DefaultLabelPosition = xaml_controls::CommandBarDefaultLabelPosition::Right;
            cmdBar->Width = 400;

            auto appBarButton1 = ref new xaml_controls::AppBarButton();
            appBarButton1->Label = L"Button1";
            appBarButton1->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Add);

            auto appBarButton2 = ref new xaml_controls::AppBarToggleButton();
            appBarButton2->Label = L"Button2";
            appBarButton2->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);

            cmdBar->PrimaryCommands->Append(appBarButton1);
            cmdBar->PrimaryCommands->Append(appBarButton2);

            auto button = ref new xaml_controls::Button();
            button->Content = L"Content Button";
            button->Width = 150;
            cmdBar->Content = button;

            auto grid = ref new xaml_controls::Grid();
            grid->Children->Append(cmdBar);
            TestServices::WindowHelper->WindowContent = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto button1 = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(0));
            button1Width = button1->ActualWidth;

            auto button2 = safe_cast<xaml_controls::AppBarToggleButton^>(cmdBar->PrimaryCommands->GetAt(1));
            button2Width = button2->ActualWidth;
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Change the CommandBar width to 200 to apply the DynamicOverflow.");
            cmdBar->Width = 200;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Now open and close the CommandBar to apply the Overflow style.");
        OpenCommandBar(cmdBar, OpenMethod::Programmatic);
        CloseCommandBar(cmdBar);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Change back the CommandBar width to 400 to move the Primary buttons from the Overflow to the pirmary commands.");
            cmdBar->Width = 400;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto button1 = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(0));
            auto button2 = safe_cast<xaml_controls::AppBarToggleButton^>(cmdBar->PrimaryCommands->GetAt(1));

            VERIFY_ARE_EQUAL(button1Width, button1->ActualWidth);
            VERIFY_ARE_EQUAL(button2Width, button2->ActualWidth);
        });
    }

    void CommandBarIntegrationTests::DoesResetOverflowButtonStylingWhenRemovedAndAddedBack()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBarButton^ button = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::AppBarButton();
            button->Label = L"button";
            button->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Accept);

            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->PrimaryCommands->Append(button);

            TestServices::WindowHelper->WindowContent = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            WEX::Common::Throw::If(ControlHelper::IsInVisualState(button, L"ApplicationViewStates", L"OverflowWithMenuIcons"), E_FAIL, L"AppBarButton should *not* have the overflow style.");

            LOG_OUTPUT(L"Size CommandBar to force the button into the overflow.");
            cmdBar->Width = 60;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // This check is removed, the visual tree doesn't exist after CommandBar default with Reveal
            // because AppBarButton reveal style is update to NULL in CFrameworkElement::LeaveImpl
            //WEX::Common::Throw::IfFalse(ControlHelper::IsInVisualState(button, L"ApplicationViewStates", L"OverflowWithMenuIcons"), E_FAIL, L"AppBarButton *should* have the overflow style.");

            LOG_OUTPUT(L"Clear the CommandBar's primary commands while the button is in the overflow.");
            cmdBar->PrimaryCommands->Clear();

            LOG_OUTPUT(L"Reset the size of the CommandBar so that we no longer will put buttons into the overflow and add the button back.");
            cmdBar->Width = std::numeric_limits<double>::quiet_NaN();
            cmdBar->PrimaryCommands->Append(button);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(ControlHelper::IsInVisualState(button, L"ApplicationViewStates", L"OverflowWithMenuIcons"));
        });
    }

    void CommandBarIntegrationTests::CanArrowIntoTheContentArea()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                        <AppBarButton Label="button" Icon="Accept"/>
                        <CommandBar.Content>
                            <Button Content="content"/>
                        </CommandBar.Content>
                    </CommandBar>)"));

            TestServices::WindowHelper->WindowContent = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Set Focus to the content area button.
        RunOnUIThread([&]()
        {
            auto button = safe_cast<xaml_controls::Button^>(cmdBar->Content);
            button->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto runScenario = [&](InputDevice inputDevice)
        {
            LOG_OUTPUT(L"Navigate Right to move focus onto the primary command button.");
            CommonInputHelper::Right(inputDevice);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(focusedElement->Equals(cmdBar->PrimaryCommands->GetAt(0)));
            });

            LOG_OUTPUT(L"Navigate Right to move focus onto the More button.");
            CommonInputHelper::Right(inputDevice);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(focusedElement->Equals(GetMoreButton(cmdBar)));
            });

            LOG_OUTPUT(L"Navigate Left to move focus onto the primary command button.");
            CommonInputHelper::Left(inputDevice);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(focusedElement->Equals(cmdBar->PrimaryCommands->GetAt(0)));
            });

            LOG_OUTPUT(L"Navigate Left to move focus onto custom content button.");
            CommonInputHelper::Left(inputDevice);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(focusedElement->Equals(cmdBar->Content));
            });
        };

        LOG_OUTPUT(L"Validate scenario for keyboard.");
        runScenario(InputDevice::Keyboard);

        LOG_OUTPUT(L"Validate scenario for gamepad.");
        runScenario(InputDevice::Gamepad);
    }

    void CommandBarIntegrationTests::DoesOpenOverflowWithArrowInputOnMoreButton()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CommandBar^ cmdBar = nullptr;

        Event openedEvent;
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);

        RunOnUIThread([&]()
        {
            cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                        <CommandBar.SecondaryCommands>
                            <AppBarButton Label="menu item 1" />
                            <AppBarButton Label="menu item 2" />
                            <AppBarButton Label="menu item 3" />
                        </CommandBar.SecondaryCommands>
                    </CommandBar>)"));

            openedRegistration.Attach(cmdBar, [&]() { openedEvent.Set(); });

            TestServices::WindowHelper->WindowContent = cmdBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Set focus to the More button.");
        RunOnUIThread([&]()
        {
            GetMoreButton(cmdBar)->Focus(xaml::FocusState::Keyboard);
        });

        LOG_OUTPUT(L"Press the Up arrow key to open the CommandBar and focus the last overflow item.");
        CommonInputHelper::Up(InputDevice::Keyboard);
        openedEvent.WaitForDefault();
        RunOnUIThread([&]()
        {
            auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_IS_TRUE(focusedElement->Equals(cmdBar->SecondaryCommands->GetAt(2)));
        });
        CloseCommandBar(cmdBar);

        LOG_OUTPUT(L"Press the Down arrow key to open the CommandBar and focus the first overflow item.");
        CommonInputHelper::Down(InputDevice::Keyboard);
        openedEvent.WaitForDefault();
        RunOnUIThread([&]()
        {
            auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_IS_TRUE(focusedElement->Equals(cmdBar->SecondaryCommands->GetAt(0)));
        });
        CloseCommandBar(cmdBar);

        // The event's HasFired flag needs to get reset before the following tests.
        openedEvent.Reset();

        LOG_OUTPUT(L"Press the Gamepad Up button and validate that the CommandBar doesn't open the overflow.");
        CommonInputHelper::Up(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(openedEvent.HasFired());

        LOG_OUTPUT(L"Press the Gamepad Down button and validate that the CommandBar doesn't open the overflow.");
        CommonInputHelper::Down(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(openedEvent.HasFired());
    }

    xaml_controls::CommandBar^ CommandBarIntegrationTests::CreateCommandBarWithPrimaryCommandOrderSet(DynamicOverflowOrderTest orderTestCase)
    {
        xaml_controls::AppBarButton^ button = nullptr;

        auto cmdBar = ref new xaml_controls::CommandBar();
        cmdBar->HorizontalAlignment = xaml::HorizontalAlignment::Center;
        cmdBar->Width = 400;

        for (int i = 0; i < 10; i++)
        {
            button = ref new xaml_controls::AppBarButton();
            button->Label = "p_btn" + (i + 1);
            cmdBar->PrimaryCommands->Append(button);
        }

        switch (orderTestCase)
        {
            case DynamicOverflowOrderTest::OrderTestForAlternativeValue: // Test for alternative order - {1,2,1,2,1,2,1,2,1,2}
            {
                for (int i = 0; i < 10; i++)
                {
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(i));
                    button->DynamicOverflowOrder = i % 2 == 0 ? 1 : 2;
                }
                break;
            }
            case DynamicOverflowOrderTest::OrderTestForAllSameValue: // Test for all same order - {1,1,1,1,1,1,1,1,1,1}
            {
                for (int i = 0; i < 10; i++)
                {
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(i));
                    button->DynamicOverflowOrder = 1;
                }
                break;
            }
            case DynamicOverflowOrderTest::OrderTestForTwoPairedValue: // Test for paired order group - {1,2,3,4,5,1,2,3,4,5}
            {
                for (int i = 0; i < 10; i++)
                {
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(i));
                    button->DynamicOverflowOrder = i % 5 + 1;
                }
                break;
            }
            case DynamicOverflowOrderTest::OrderTestForFallbackDefault: // Test for order set and default rightmost dynamic overflow - {1,1,2,2,0,0,0,0,0,0}
            {
                for (int i = 0; i < 10; i++)
                {
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(i));
                    if (i < 2)
                    {
                        button->DynamicOverflowOrder = 1;
                    }
                    else if (i < 4)
                    {
                        button->DynamicOverflowOrder = 2;
                    }
                }
                break;
            }
            case DynamicOverflowOrderTest::OrderTestForMovingPriorSeparator: // Test for separator moving that is in the prior index of moving primary command
            {
                for (int i = 0; i < 10; i++)
                {
                    button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(i));
                    button->DynamicOverflowOrder = i % 5 + 1;
                }
                auto separator1 = ref new xaml_controls::AppBarSeparator();
                cmdBar->PrimaryCommands->InsertAt(1, separator1);
                auto separator2 = ref new xaml_controls::AppBarSeparator();
                cmdBar->PrimaryCommands->InsertAt(0, separator2);
                break;
            }
            case DynamicOverflowOrderTest::OrderTestForMovingPosteriorSeparator: // Test for separator moving that is in the posterior index of moving primary command
            {
                button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(9));
                button->DynamicOverflowOrder = 1;
                auto separator1 = ref new xaml_controls::AppBarSeparator();
                cmdBar->PrimaryCommands->InsertAt(10, separator1);
                auto separator2 = ref new xaml_controls::AppBarSeparator();
                cmdBar->PrimaryCommands->InsertAt(9, separator2);
                break;
            }
        }

        return cmdBar;
    }

    xaml_controls::Page^ CommandBarIntegrationTests::SetupDynamicOverflowTest(xaml_controls::CommandBar^* commandBar, unsigned int numButtonsToAddExtraToPrimary, unsigned int numButtonsToAddToSecondary, bool isSetOrder)
    {
        xaml_controls::Page^ page = nullptr;
        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            *commandBar = cmdBar;

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = cmdBar;

            auto button = ref new xaml_controls::AppBarButton();
            cmdBar->PrimaryCommands->Append(button);

        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto button = safe_cast<xaml_controls::AppBarButton^>(cmdBar->PrimaryCommands->GetAt(0));
            button->Label = "p_btn0";

            auto moreButton = GetMoreButton(cmdBar);

            LOG_OUTPUT(L"CommandBar ActualWidth = %f", cmdBar->ActualWidth);
            LOG_OUTPUT(L"AppBarButton ActualWidth = %f", button->ActualWidth);
            LOG_OUTPUT(L"MoreButton ActualWidth = %f", moreButton->ActualWidth);

            VERIFY_ARE_NOT_EQUAL(0, button->ActualWidth);

            unsigned int numButtonsToStayOnPrimary = static_cast<unsigned int>((cmdBar->ActualWidth - moreButton->ActualWidth) / button->ActualWidth);
            unsigned int numButtonsToAddToPrimary = numButtonsToStayOnPrimary + numButtonsToAddExtraToPrimary - 1;

            for (unsigned int i = 0; i < numButtonsToAddToPrimary; ++i)
            {
                button = ref new xaml_controls::AppBarButton();
                button->Label = "p_btn" + (i + 1);
                if (isSetOrder)
                {
                    button->DynamicOverflowOrder = i;
                }
                cmdBar->PrimaryCommands->Append(button);
            }

            for (unsigned int i = 0; i < numButtonsToAddToSecondary; ++i)
            {
                button = ref new xaml_controls::AppBarButton();
                button->Label = "s_btn" + i;
                cmdBar->SecondaryCommands->Append(button);
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        return page;
    }

    xaml_controls::Control^ CommandBarIntegrationTests::GetMoreButton(xaml_controls::CommandBar^ cmdBar)
    {
        xaml_controls::Control^ moreButton;
        RunOnUIThread([&]()
        {
            moreButton = safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton"));
        });

        return moreButton;
    }

    void CommandBarIntegrationTests::OpenCommandBar(xaml_controls::CommandBar^ cmdBar, OpenMethod openMethod)
    {
        Event openedEvent;
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        openedRegistration.Attach(cmdBar, [&]() { openedEvent.Set(); });
        xaml_controls::Control^ moreButton = GetMoreButton(cmdBar);

        if (openMethod == OpenMethod::Mouse)
        {
            TestServices::InputHelper->LeftMouseClick(moreButton);
        }
        else if (openMethod == OpenMethod::Touch)
        {
            TestServices::InputHelper->Tap(moreButton);
        }
        else if (openMethod == OpenMethod::Keyboard)
        {
            RunOnUIThread([&]()
            {
                moreButton->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L" ");
        }
        else if (openMethod == OpenMethod::Gamepad)
        {
            RunOnUIThread([&]()
            {
                moreButton->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            CommonInputHelper::Accept(InputDevice::Gamepad);
        }
        else if (openMethod == OpenMethod::Programmatic)
        {
            RunOnUIThread([&]()
            {
                cmdBar->IsOpen = true;
            });
        }

        openedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::CloseCommandBar(xaml_controls::CommandBar^ cmdBar)
    {
        Event closedEvent;
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        closedRegistration.Attach(cmdBar, [&]() { closedEvent.Set(); });

        RunOnUIThread([&]()
        {
            cmdBar->IsOpen = false;
        });
        closedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::EmptyPageContent(xaml_controls::Page^ page)
    {
        RunOnUIThread([&]()
        {
            page->TopAppBar = nullptr;
            page->BottomAppBar = nullptr;
            page->Content = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::ValidateSymbolMenuIcons()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating Symbol Menu Icons in the Overflow window");

        Platform::String^ xamlString = LR"(<SymbolIcon xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" Symbol="Accept" />)";

        RunSecondaryIconTests(xamlString);
    }

    void CommandBarIntegrationTests::ValidatePathMenuIcons()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating Path Menu Icons in the Overflow window");

        Platform::String^ xamlString = LR"(<PathIcon xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" Data="F1 M 16,12 20,2L 20,16 1,16" HorizontalAlignment="Center" />)";

        RunSecondaryIconTests(xamlString);
    }

    void CommandBarIntegrationTests::ValidateFontMenuIcons()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating Font Menu Icons in the Overflow window");

        Platform::String^ xamlString = LR"(<FontIcon xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" FontFamily="Candara" Glyph="&#x03A3;" />)";

        RunSecondaryIconTests(xamlString);
    }

    void CommandBarIntegrationTests::ValidateBitmapMenuIcons()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating Bitmap Menu Icons in the Overflow window");

        Platform::String^ xamlString = LR"(<BitmapIcon xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" UriSource="ms-appx:///resources/native/controls/commandbar/bitimg.png" />)";

        RunSecondaryIconTests(xamlString);
    }

    void CommandBarIntegrationTests::ValidateBitmapMenuIconsNoMonochrome()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating Bitmap Menu Icons in the Overflow window");

        Platform::String^ xamlString = LR"(<BitmapIcon xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" UriSource="ms-appx:///resources/native/controls/commandbar/bitimg.png" ShowAsMonochrome="False" />)";

        RunSecondaryIconTests(xamlString);
    }

    void CommandBarIntegrationTests::RunSecondaryIconTests(Platform::String^ xamlString)
    {
        ControlHelper::ValidateUIElementTree(
            PopupHelper::AreWindowedPopupsEnabled() ? L"Windowed" : L"Unwindowed",
            wf::Size(400, 600),
            1.f,
            [&]()
        {
            xaml_controls::Grid^ root;
            xaml_controls::CommandBar^ cmdBar;
            xaml_controls::AppBarButton^ appBarButton;

            RunOnUIThread([&]()
            {
                root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                    LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                                  xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                                <CommandBar x:Name="cmdBar" VerticalAlignment="Bottom" HorizontalAlignment="Center" >
                                    <CommandBar.PrimaryCommands>
                                        <AppBarButton x:Name="appBarButton1" Label="Item 1" Icon="Accept"/>
                                    </CommandBar.PrimaryCommands>
                                    <CommandBar.SecondaryCommands>
                                        <AppBarButton x:Name="appBarButton2" Label="Button 1" />
                                        <AppBarToggleButton x:Name="appBarToggleButton1" Label="Button 2" />
                                    </CommandBar.SecondaryCommands>
                                </CommandBar>
                            </Grid>)"));

                auto icon = safe_cast<xaml_controls::IconElement^>(xaml_markup::XamlReader::Load(xamlString));

                cmdBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"cmdBar"));
                appBarButton = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"appBarButton2"));

                appBarButton->Icon = icon;

                root->IsHitTestVisible = false;

                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // The tool tip on the CommandBar can annoyingly inject itself into the UI tree dump
                // if it happens to have opened.  We'll remove it to avoid that circumstance.
                auto moreButton = TreeHelper::GetVisualChildByName(cmdBar, L"MoreButton");
                xaml_controls::ToolTipService::SetToolTip(moreButton, nullptr);
            });

            TestServices::WindowHelper->WaitForIdle();

            OpenCommandBar(cmdBar, OpenMethod::Programmatic);

            TestServices::WindowHelper->WaitForIdle();

            return root;
        });
    }


    void CommandBarIntegrationTests::ValidateForegroundForXamlUICommand()
    {
        TestCleanupWrapper cleanup;

        // Because of XamlUICommand, IconElement is used to convert IconSource to Icon. If no Foreground is set,
        // We expect IconElement Foreground is from its parent AppBarButton/AppBarButton
        // In this way, the visualstate of AppBarButton/AppBarButton like Disabled would apply to IconElement
        xaml_controls::Grid^ root;
        xaml_controls::AppBarButton^ button1;
        xaml_controls::AppBarButton^ buttonWithCommand1;
        xaml_controls::AppBarToggleButton^ buttonWithCommand2;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                                xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                            <Grid.Resources>
                                <XamlUICommand x:Key="AcceptCommand" Label="Accept">
                                    <XamlUICommand.IconSource>
                                        <SymbolIconSource Symbol="Accept"/>
                                    </XamlUICommand.IconSource>
                                </XamlUICommand>
                                <XamlUICommand x:Key="FavoriteCommand" Label="Favorite">
                                    <XamlUICommand.IconSource>
                                        <SymbolIconSource Foreground="Red" Symbol="Favorite" />
                                    </XamlUICommand.IconSource>
                                </XamlUICommand>
                            </Grid.Resources>
                            <CommandBar DefaultLabelPosition="Right" Foreground="Blue">
                                <AppBarButton x:Name="Button1" IsEnabled="False" Label="Accept" >
                                    <AppBarButton.Icon>
                                        <IconSourceElement>
                                            <SymbolIconSource Symbol="Favorite" Foreground="Red" />
                                        </IconSourceElement>
                                    </AppBarButton.Icon>
                                </AppBarButton>
                                <AppBarButton x:Name="ButtonWithCommand1" IsEnabled="False" Command="{StaticResource AcceptCommand}" />
                                <AppBarToggleButton x:Name="ButtonWithCommand2" IsEnabled="False" Command="{StaticResource FavoriteCommand}" />
                            </CommandBar>
                        </Grid>)"));

            TestServices::WindowHelper->WindowContent = root;

            button1 = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"Button1"));
            buttonWithCommand1 = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"ButtonWithCommand1"));
            buttonWithCommand2 = safe_cast<xaml_controls::AppBarToggleButton^>(root->FindName(L"ButtonWithCommand2"));
            loadedRegistration.Attach(root, [&] { loadedEvent->Set(); });
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Button1.IconSourceElement has it's own Foreground");
            auto iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(button1->Icon);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::SolidColorBrush^>(iconSourceElement->IconSource->Foreground)->Color, Colors::Red);

            LOG_OUTPUT(L"buttonWithCommand1.IconSourceElement doesn't have it's own Foreground");
            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(buttonWithCommand1->Icon);
            VERIFY_ARE_EQUAL(iconSourceElement->IconSource->Foreground, nullptr);

            LOG_OUTPUT(L"buttonWithCommand2.IconSourceElement has it's own Foreground");
            iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(buttonWithCommand2->Icon);
            VERIFY_ARE_EQUAL(safe_cast<xaml_media::SolidColorBrush^>(iconSourceElement->IconSource->Foreground)->Color, Colors::Red);
        });
    }

    void CommandBarIntegrationTests::ValidateCollapsedItemsDoNotPreventReturnFromOverflow()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ appBarButton = nullptr;

        bool expectItemsAdded = false;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            cmdBar->Width = 600;

            appBarButton = ref new xaml_controls::AppBarButton();
            appBarButton->Label = L"Button 1";
            appBarButton->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::AddFriend);
            cmdBar->PrimaryCommands->Append(appBarButton);

            auto button = ref new xaml_controls::Button();
            button->Content = L"Content Button";
            button->Width = 400;
            cmdBar->Content = button;

            auto grid = ref new xaml_controls::Grid();
            grid->Children->Append(cmdBar);
            loadedRegistration.Attach(grid, [&] { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = grid;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            appBarButton->Visibility = xaml::Visibility::Collapsed;
        });

        TestServices::WindowHelper->WaitForIdle();

        auto dynamicOverflowItemsChangingEvent = std::make_shared<Event>();
        auto dynamicOverflowItemsChangingRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, DynamicOverflowItemsChanging);

        dynamicOverflowItemsChangingRegistration.Attach(cmdBar,
            ref new wf::TypedEventHandler<xaml_controls::CommandBar^, xaml_controls::DynamicOverflowItemsChangingEventArgs^>(
            [&](xaml_controls::CommandBar^ sender, xaml_controls::DynamicOverflowItemsChangingEventArgs^ args)
            {
                if (expectItemsAdded)
                {
                    VERIFY_ARE_EQUAL(xaml_controls::CommandBarDynamicOverflowAction::AddingToOverflow, args->Action);
                }
                else
                {
                    VERIFY_ARE_EQUAL(xaml_controls::CommandBarDynamicOverflowAction::RemovingFromOverflow, args->Action);
                }

                dynamicOverflowItemsChangingEvent->Set();
            }));

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Change the width of the CommandBar to be 300.  The AppBarButton should be moved to the overflow.");
            expectItemsAdded = true;
            cmdBar->Width = 300;
        });

        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Change the width of the CommandBar back to 600.  The AppBarButton should be moved back from the overflow.");
            expectItemsAdded = false;
            cmdBar->Width = 600;
        });

        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::ValidateMoreButtonCanShowWithoutSizeChanging()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_primitives::ButtonBase^ moreButton = nullptr;

        auto hasLoadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);

        RunOnUIThread([&]()
        {
            cmdBar = ref new xaml_controls::CommandBar();
            loadedRegistration.Attach(cmdBar, [&]() { hasLoadedEvent->Set(); });

            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = cmdBar;
        });

        hasLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            moreButton = safe_cast<xaml_primitives::ButtonBase^>(GetMoreButton(cmdBar));
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, moreButton->Visibility);

            cmdBar->SecondaryCommands->Append(ref new xaml_controls::AppBarButton());
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, moreButton->Visibility);

            cmdBar->SecondaryCommands->Clear();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, moreButton->Visibility);

            cmdBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Minimal;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, moreButton->Visibility);

            cmdBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Compact;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, moreButton->Visibility);

            cmdBar->OverflowButtonVisibility = xaml_controls::CommandBarOverflowButtonVisibility::Visible;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, moreButton->Visibility);

            cmdBar->OverflowButtonVisibility = xaml_controls::CommandBarOverflowButtonVisibility::Auto;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, moreButton->Visibility);
        });
    }

    void CommandBarIntegrationTests::ValidateButtonsMoveToAndFromOverflowWithoutSizeChange()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootGrid = nullptr;
        xaml_controls::ColumnDefinition^ primaryColumn = nullptr;

        auto sizeChangedEvent = std::make_shared<Event>();
        auto dynamicOverflowItemsChangingEvent = std::make_shared<Event>();
        auto sizeChangedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, SizeChanged);
        auto dynamicOverflowItemsChangingRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, DynamicOverflowItemsChanging);

        auto expectItemAdded = std::make_shared<bool>();

        RunOnUIThread([&]()
        {
            rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Grid>
                            <Grid.ColumnDefinitions>
                                <ColumnDefinition x:Name='PrimaryColumn' Width='320' />
                                <ColumnDefinition />
                            </Grid.ColumnDefinitions>
                            <Grid>
                                <CommandBar HorizontalAlignment='Right' x:Name='CommandBar'>
                                    <AppBarButton Icon='Add' Label='Add'/>
                                </CommandBar>
                            </Grid>
                            <Border Grid.Column='1' Background='Red' />
                        </Grid>
                    </Grid>)"));

            auto commandBar = safe_cast<xaml_controls::CommandBar^>(rootGrid->FindName(L"CommandBar"));

            dynamicOverflowItemsChangingRegistration.Attach(
                commandBar, ref new wf::TypedEventHandler<xaml_controls::CommandBar^, xaml_controls::DynamicOverflowItemsChangingEventArgs^>(
                    [dynamicOverflowItemsChangingEvent, expectItemAdded](Platform::Object^ /*sender*/, xaml_controls::DynamicOverflowItemsChangingEventArgs^ args)
            {
                if (*expectItemAdded)
                {
                    VERIFY_ARE_EQUAL(xaml_controls::CommandBarDynamicOverflowAction::AddingToOverflow, args->Action);
                }
                else
                {
                    VERIFY_ARE_EQUAL(xaml_controls::CommandBarDynamicOverflowAction::RemovingFromOverflow, args->Action);
                }

                dynamicOverflowItemsChangingEvent->Set();
            }));

            primaryColumn = safe_cast<xaml_controls::ColumnDefinition^>(rootGrid->FindName(L"PrimaryColumn"));

            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        TestServices::WindowHelper->WaitForIdle();

        *expectItemAdded = true;

        RunOnUIThread([&]()
        {
            primaryColumn->Width = GridLengthHelper::FromPixels(0);
        });

        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(1, dynamicOverflowItemsChangingEvent->TimesFired());

        *expectItemAdded = false;

        RunOnUIThread([&]()
        {
            primaryColumn->Width = GridLengthHelper::FromPixels(320);
        });

        dynamicOverflowItemsChangingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(2, dynamicOverflowItemsChangingEvent->TimesFired());
    }

    void CommandBarIntegrationTests::VerifyLightDismissWithScaling()
    {
        // Set the scale factor to 125%:
        ChangeDPI changeDPI;

        TestCleanupWrapper cleanup;

        // Regression coverage for:
        // Can't always close CommandBar overflow menu with windows scaling > 100%
        // The issue is that CommandBar's light dismiss overlay element was not scaling to the full window when a scale factor is applied.
        // To test, we open a CommandBar and tap in the bottom right corner of the window and ensure that the CommandBar is dismissed.

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 400.0f));

        xaml_controls::CommandBar^ cmdBar;
        xaml_shapes::Rectangle^ elementToTap;

        Event closedEvent;
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                          Width="400.0" Height="400.0"
                          HorizontalAlignment='Left'
                          VerticalAlignment='Top'>
                        <CommandBar x:Name="cmdBar" VerticalAlignment="Top" >
                            <AppBarButton Label="Item 1" Icon="Pause"/>
                            <CommandBar.SecondaryCommands >
                                <AppBarButton Label="Menu1" />
                                <AppBarButton Label="Menu2" />
                                <AppBarButton Label="Menu3" />
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                        <Rectangle x:Name="elementToTap" VerticalAlignment="Bottom" HorizontalAlignment="Right" Width="10" Height="10" Fill="Red" />
                    </Grid>)"));

            cmdBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"cmdBar"));
            elementToTap = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"elementToTap"));

            closedRegistration.Attach(cmdBar, [&]() { closedEvent.Set(); });

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        OpenCommandBar(cmdBar, OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        // Tap on the bottom right and verify that the CommandBar closes.
        TestServices::InputHelper->Tap(elementToTap);
        TestServices::WindowHelper->WaitForIdle();
        closedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::VerifyVisibilityChangeUpdatesCommandBarVisualState()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::AppBarButton^ appBarButton = nullptr;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <CommandBar x:Name="cmdBar" VerticalAlignment="Top" >
                            <AppBarButton Label="Item 1" Name="button" Visibility="Collapsed"/>
                            <CommandBar.SecondaryCommands>
                                <AppBarButton Label="Item 2"/>
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                    </Grid>)"));

            cmdBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"cmdBar"));
            appBarButton = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"button"));

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(cmdBar, L"AvailableCommandsStates", L"SecondaryCommandsOnly"));
            appBarButton->Visibility = xaml::Visibility::Visible;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(cmdBar, L"AvailableCommandsStates", L"BothCommands"));
        });
    }

    // When AppBarButton/AppBarButton is collapsed, CommandBar is notified in AppBarButton/AppBarToggleButton::OnVisibilityChanged. and ButtonBase::OnVisibilityChanged would ClearStateFlags.
    // AppBarButton/AppBarToggleButton is a subclass of ButtonBase and it should call super::OnVisibilityChanged to ClearStateFlags
    // In this test case, when AppBarButton/AppBarButton is clicked, it collapse itself. then show them all when click approve button. then verify ClearStateFlags are called by checking IsPointerOver flag.
    void CommandBarIntegrationTests::ValidateResetingTheStateOfAppBarButton()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ root;
        xaml_controls::CommandBar^ cmdBar;
        xaml_controls::AppBarButton^ button;
        xaml_controls::AppBarToggleButton^ toggleButton;

        xaml_controls::AppBarButton^ approveButton;
        auto buttonEventRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, Click);
        auto toggleButtonEventRegistration = CreateSafeEventRegistration(xaml_controls::AppBarToggleButton, Click);
        auto approveButtonEventRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, Click);

        auto buttonEvent = std::make_shared<Event>();
        auto toggleButtonEvent = std::make_shared<Event>();
        auto approveButtonEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                              xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <CommandBar x:Name="cmdBar">
                            <AppBarButton x:Name="Button" Icon="Add"/>
                            <AppBarToggleButton x:Name="ToggleButton" Icon="AddFriend" />
                            <AppBarButton IsEnabled="False" Icon="Cancel" Label="PlaceHolder" />
                            <AppBarButton x:Name="ApproveButton" Icon="Accept" />
                        </CommandBar>
                    </Grid>)"));

            cmdBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"cmdBar"));
            button = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"Button"));
            toggleButton = safe_cast<xaml_controls::AppBarToggleButton^>(root->FindName(L"ToggleButton"));
            approveButton = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"ApproveButton"));

            TestServices::WindowHelper->WindowContent = root;
        });

        buttonEventRegistration.Attach(button, [&]()
        {
            button->Visibility = xaml::Visibility::Collapsed;
            buttonEvent->Set();
        });

        toggleButtonEventRegistration.Attach(toggleButton, [&]()
        {
            toggleButton->Visibility = xaml::Visibility::Collapsed;
            toggleButtonEvent->Set();
        });
        approveButtonEventRegistration.Attach(approveButton, [&]()
        {
            button->Visibility = xaml::Visibility::Visible;
            toggleButton->Visibility = xaml::Visibility::Visible;
            approveButtonEvent->Set();
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->LeftMouseClick(button);
        buttonEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->LeftMouseClick(toggleButton);
        toggleButtonEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->LeftMouseClick(approveButton);
        approveButtonEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify buttons PointerOver flag is removed
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(button->IsPointerOver);
            VERIFY_IS_FALSE(toggleButton->IsPointerOver);
        });
    }

    void CommandBarIntegrationTests::VerifyCanMakeSubMenuBySettingFlyoutProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ flyoutButton;
        xaml_controls::Flyout^ commandBarFlyout;
        xaml_controls::CommandBar^ commandBar;
        xaml_controls::AppBarButton^ menuFlyoutAppBarButton1;
        xaml_controls::AppBarButton^ menuFlyoutAppBarButton2;
        xaml_controls::AppBarButton^ menuFlyoutAppBarButton3;
        xaml_controls::AppBarButton^ appBarButtonWithoutFlyout;
        xaml_controls::AppBarToggleButton^ appBarToggleButton;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem1;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem2;
        xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem;
        xaml_controls::MenuFlyoutItem^ menuFlyoutItem3;

        auto menuFlyoutAppBarButton1LoadedEvent = std::make_shared<Event>();
        auto menuFlyoutAppBarButton1LoadedEventRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, Loaded);
        auto commandBarOpenedEvent = std::make_shared<Event>();
        auto commandBarOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto commandBarClosedEvent = std::make_shared<Event>();
        auto commandBarClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto menuFlyout1OpenedEvent = std::make_shared<Event>();
        auto menuFlyout1OpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyout1ClosedEvent = std::make_shared<Event>();
        auto menuFlyout1ClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);
        auto menuFlyout2OpenedEvent = std::make_shared<Event>();
        auto menuFlyout2OpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyout2ClosedEvent = std::make_shared<Event>();
        auto menuFlyout2ClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);
        auto menuFlyout3OpenedEvent = std::make_shared<Event>();
        auto menuFlyout3OpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyout3ClosedEvent = std::make_shared<Event>();
        auto menuFlyout3ClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);
        auto menuFlyoutItem1ClickEvent = std::make_shared<Event>();
        auto menuFlyoutItem1ClickEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);
        auto menuFlyoutItem2ClickEvent = std::make_shared<Event>();
        auto menuFlyoutItem2ClickEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

        RunOnUIThread([&]()
        {
            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();

            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                          Background="Transparent">
                        <Button x:Name="FlyoutButton" Content="Click for flyout">
                            <Button.Flyout>
                                <Flyout x:Name="CommandBarFlyout">
                                    <CommandBar x:Name="CommandBar" IsOpen="True" Width="300">
                                        <CommandBar.SecondaryCommands>
                                            <AppBarButton x:Name="MenuFlyoutAppBarButton1" Label="Open menu flyout">
                                                <AppBarButton.Flyout>
                                                    <MenuFlyout x:Name="MenuFlyout1">
                                                        <MenuFlyoutItem x:Name="TestMenuFlyoutItem1" Text="Click me" />
                                                    </MenuFlyout>
                                                </AppBarButton.Flyout>
                                            </AppBarButton>
                                            <AppBarButton x:Name="MenuFlyoutAppBarButton2" Label="Open other menu flyout">
                                                <AppBarButton.Flyout>
                                                    <MenuFlyout x:Name="MenuFlyout2">
                                                        <MenuFlyoutSubItem x:Name="TestMenuFlyoutSubItem" Text="Sub-menu item">
                                                            <MenuFlyoutItem x:Name="TestMenuFlyoutItem2" Text="Click me" />
                                                        </MenuFlyoutSubItem>
                                                    </MenuFlyout>
                                                </AppBarButton.Flyout>
                                            </AppBarButton>
                                            <AppBarButton x:Name="MenuFlyoutAppBarButton3" Label="Open third menu flyout">
                                                <AppBarButton.Flyout>
                                                    <MenuFlyout x:Name="MenuFlyout3">
                                                        <MenuFlyoutItem x:Name="TestMenuFlyoutItem3" Text="Click me" />
                                                    </MenuFlyout>
                                                </AppBarButton.Flyout>
                                            </AppBarButton>
                                            <AppBarButton x:Name="AppBarButtonWithoutFlyout" Label="No menu flyout" />
                                            <AppBarToggleButton x:Name="AppBarToggleButton" Label="No menu flyout" />
                                        </CommandBar.SecondaryCommands>
                                    </CommandBar>
                                </Flyout>
                            </Button.Flyout>
                        </Button>
                    </Grid>)"));

            flyoutButton = safe_cast<xaml_controls::Button^>(root->FindName(L"FlyoutButton"));
            commandBarFlyout = safe_cast<xaml_controls::Flyout^>(root->FindName(L"CommandBarFlyout"));
            commandBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"CommandBar"));
            menuFlyoutAppBarButton1 = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"MenuFlyoutAppBarButton1"));
            menuFlyoutItem1 = safe_cast<xaml_controls::MenuFlyoutItem^>(root->FindName(L"TestMenuFlyoutItem1"));
            menuFlyoutAppBarButton2 = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"MenuFlyoutAppBarButton2"));
            menuFlyoutSubItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(root->FindName(L"TestMenuFlyoutSubItem"));
            menuFlyoutItem2 = safe_cast<xaml_controls::MenuFlyoutItem^>(root->FindName(L"TestMenuFlyoutItem2"));
            menuFlyoutAppBarButton3 = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"MenuFlyoutAppBarButton3"));
            menuFlyoutItem3 = safe_cast<xaml_controls::MenuFlyoutItem^>(root->FindName(L"TestMenuFlyoutItem3"));
            appBarButtonWithoutFlyout = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"AppBarButtonWithoutFlyout"));
            appBarToggleButton = safe_cast<xaml_controls::AppBarToggleButton^>(root->FindName(L"AppBarToggleButton"));

            commandBarOpenedEventRegistration.Attach(commandBar, [&]()
            {
                LOG_OUTPUT(L"CommandBar opened.");
                commandBarOpenedEvent->Set();
            });

            commandBarClosedEventRegistration.Attach(commandBar, [&]()
            {
                LOG_OUTPUT(L"CommandBar closed.");
                commandBarClosedEvent->Set();
            });

            menuFlyoutAppBarButton1LoadedEventRegistration.Attach(menuFlyoutAppBarButton1, [&]()
            {
                LOG_OUTPUT(L"MenuFlyoutAppBarButton1 loaded.");
                menuFlyoutAppBarButton1LoadedEvent->Set();
            });

            menuFlyoutItem1ClickEventRegistration.Attach(menuFlyoutItem1, [&]()
            {
                LOG_OUTPUT(L"MenuFlyoutItem1 clicked.");
                menuFlyoutItem1ClickEvent->Set();
            });

            menuFlyoutItem2ClickEventRegistration.Attach(menuFlyoutItem2, [&]()
            {
                LOG_OUTPUT(L"MenuFlyoutItem2 clicked.");
                menuFlyoutItem2ClickEvent->Set();
            });

            auto menuFlyout1 = safe_cast<xaml_controls::MenuFlyout^>(root->FindName(L"MenuFlyout1"));

            menuFlyout1OpenedEventRegistration.Attach(menuFlyout1, [&]()
            {
                LOG_OUTPUT(L"MenuFlyout1 opened.");
                menuFlyout1OpenedEvent->Set();
            });

            menuFlyout1ClosedEventRegistration.Attach(menuFlyout1, [&]()
            {
                LOG_OUTPUT(L"MenuFlyout1 closed.");
                menuFlyout1ClosedEvent->Set();
            });

            auto menuFlyout2 = safe_cast<xaml_controls::MenuFlyout^>(root->FindName(L"MenuFlyout2"));

            menuFlyout2OpenedEventRegistration.Attach(menuFlyout2, [&]()
            {
                LOG_OUTPUT(L"MenuFlyout2 opened.");
                menuFlyout2OpenedEvent->Set();
            });

            menuFlyout2ClosedEventRegistration.Attach(menuFlyout2, [&]()
            {
                LOG_OUTPUT(L"MenuFlyout2 closed.");
                menuFlyout2ClosedEvent->Set();
            });

            auto menuFlyout3 = safe_cast<xaml_controls::MenuFlyout^>(root->FindName(L"MenuFlyout3"));

            menuFlyout3OpenedEventRegistration.Attach(menuFlyout3, [&]()
            {
                LOG_OUTPUT(L"MenuFlyout3 opened.");
                menuFlyout3OpenedEvent->Set();
            });

            menuFlyout3ClosedEventRegistration.Attach(menuFlyout3, [&]()
            {
                LOG_OUTPUT(L"MenuFlyout3 closed.");
                menuFlyout3ClosedEvent->Set();
            });

            page->Content = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(commandBarFlyout, flyoutButton, FlyoutOpenMethod::Programmatic_ShowAt);

        // Since the CommandBar starts with IsOpen already true, we don't get an Opened event the first time.
        // We'll listen for the loaded event on MenuFlyoutAppBarButton1 instead.
        menuFlyoutAppBarButton1LoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over MenuFlyoutAppBarButton1, which should open the first menu flyout.");
        TestServices::InputHelper->MoveMouse(menuFlyoutAppBarButton1);
        menuFlyout1OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Clicking on the MenuFlyoutItem, which should close both the menu flyout and the CommandBar.");
        TestServices::InputHelper->LeftMouseClick(menuFlyoutItem1);
        menuFlyoutItem1ClickEvent->WaitForDefault();
        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar->IsOpen = true;
        });

        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over MenuFlyoutAppBarButton2, which should close the first menu flyout and open the second.");
        TestServices::InputHelper->MoveMouse(menuFlyoutAppBarButton2);
        menuFlyout1ClosedEvent->WaitForDefault();
        menuFlyout2OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over MenuFlyoutSubItem, which should open the sub-menu.");
        TestServices::InputHelper->MoveMouse(menuFlyoutSubItem);
        // Wait for the sub menu to open. It opens after a delay - clicking and waiting for idle doesn't open it.
        // <MenuFlyout sub items don't expand on mouse click - they need to wait for the timeout> tracks this bug.
        TestServices::WindowHelper->SynchronouslyTickUIThread(60);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Clicking on the MenuFlyoutItem, which should close both the menu flyout and the CommandBar.");
        TestServices::InputHelper->LeftMouseClick(menuFlyoutItem2);
        menuFlyoutItem2ClickEvent->WaitForDefault();
        menuFlyout2ClosedEvent->WaitForDefault();
        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar->IsOpen = true;
        });

        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over MenuFlyoutAppBarButton3, which should open the third menu flyout.");
        TestServices::InputHelper->MoveMouse(menuFlyoutAppBarButton3);
        menuFlyout3OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over MenuFlyoutItem3, then over AppBarButtonWithoutFlyout, which should close the third menu flyout.");
        TestServices::InputHelper->MoveMouse(menuFlyoutItem3);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MoveMouse(appBarButtonWithoutFlyout);
        menuFlyout3ClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over MenuFlyoutAppBarButton3, which should open the third menu flyout.");
        TestServices::InputHelper->MoveMouse(menuFlyoutAppBarButton3);
        menuFlyout3OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over MenuFlyoutItem3, then over AppBarToggleButton, which should close the third menu flyout.");
        TestServices::InputHelper->MoveMouse(menuFlyoutItem3);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MoveMouse(appBarToggleButton);
        menuFlyout3ClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar->IsOpen = false;
        });

        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(commandBarFlyout);
    }

    void CommandBarIntegrationTests::VerifySubMenuDoesNotEatPointerInput()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ commandBar;
        xaml_controls::AppBarButton^ flyoutAppBarButton;
        xaml_controls::AppBarButton^ otherAppBarButton;

        auto commandBarOpenedEvent = std::make_shared<Event>();
        auto commandBarOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto commandBarClosedEvent = std::make_shared<Event>();
        auto commandBarClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();

            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                          Background="Transparent">
                        <CommandBar x:Name="CommandBar" Width="300">
                            <CommandBar.SecondaryCommands>
                                <AppBarButton x:Name="FlyoutAppBarButton" Label="Open flyout">
                                    <AppBarButton.Flyout>
                                        <MenuFlyout x:Name="MenuFlyout">
                                            <MenuFlyoutItem Text="Click me" />
                                        </MenuFlyout>
                                    </AppBarButton.Flyout>
                                </AppBarButton>
                                <AppBarButton x:Name="OtherAppBarButton" Label="Other AppBarButton" />
                            </CommandBar.SecondaryCommands>
                        </CommandBar>
                    </Grid>)"));

            commandBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"CommandBar"));
            flyoutAppBarButton = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"FlyoutAppBarButton"));
            otherAppBarButton = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"OtherAppBarButton"));

            auto menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(root->FindName(L"MenuFlyout"));

            commandBarOpenedEventRegistration.Attach(commandBar, [&]() { commandBarOpenedEvent->Set(); });
            commandBarClosedEventRegistration.Attach(commandBar, [&]() { commandBarClosedEvent->Set(); });
            menuFlyoutOpenedEventRegistration.Attach(menuFlyout, [&]() { menuFlyoutOpenedEvent->Set(); });
            menuFlyoutClosedEventRegistration.Attach(menuFlyout, [&]() { menuFlyoutClosedEvent->Set(); });

            page->Content = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar->IsOpen = true;
        });

        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over FlyoutAppBarButton, which should open the flyout.");
        TestServices::InputHelper->MoveMouse(flyoutAppBarButton);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving mouse over OtherAppBarButton, which should close the flyout.");
        TestServices::InputHelper->MoveMouse(otherAppBarButton);
        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar->IsOpen = false;
        });

        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::VerifySubMenuHasLightDismissOnPrimaryAppBarButton()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ commandBar;
        xaml_controls::AppBarButton^ flyoutAppBarButton;

        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();

            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                          Background="Transparent">
                        <CommandBar x:Name="CommandBar" Width="300">
                            <AppBarButton x:Name="FlyoutAppBarButton" Label="Open flyout">
                                <AppBarButton.Flyout>
                                    <MenuFlyout x:Name="MenuFlyout">
                                        <MenuFlyoutItem Text="Click me" />
                                    </MenuFlyout>
                                </AppBarButton.Flyout>
                            </AppBarButton>
                        </CommandBar>
                    </Grid>)"));

            commandBar = safe_cast<xaml_controls::CommandBar^>(root->FindName(L"CommandBar"));
            flyoutAppBarButton = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"FlyoutAppBarButton"));

            auto menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(root->FindName(L"MenuFlyout"));

            menuFlyoutOpenedEventRegistration.Attach(menuFlyout, [&]() { menuFlyoutOpenedEvent->Set(); });
            menuFlyoutClosedEventRegistration.Attach(menuFlyout, [&]() { menuFlyoutClosedEvent->Set(); });

            page->Content = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping on FlyoutAppBarButton, which should open the flyout.");
        TestServices::InputHelper->Tap(flyoutAppBarButton);
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping in empty space, which should close the flyout.");
        TestServices::InputHelper->Tap({ 300, 300 });
        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::VerifyAppBarButtonFlyoutPosition()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 800));

        auto rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetPackageFolder()
            + L"resources\\native\\controls\\commandbar\\CommandBarTest.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);

        auto commandBarOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto commandBarOpenedEvent = std::make_shared<Event>();

        auto commandBarClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto commandBarClosedEvent = std::make_shared<Event>();

        auto menuFlyoutOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyoutOpenedEvent = std::make_shared<Event>();

        auto menuFlyoutClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);
        auto menuFlyoutClosedEvent = std::make_shared<Event>();

        auto gotFocusEventRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
        auto gotFocusEvent = std::make_shared<Event>();

        xaml_controls::CommandBar^ commandBar = nullptr;
        xaml_controls::AppBarButton^ appBarButton = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::Button^ moreButton = nullptr;

        wf::Point menuFlyoutPosition1 = {}, menuFlyoutPosition2 = {};

        RunOnUIThread([&]()
        {
            loadedEventRegistration.Attach(rootPage, [&]
            {
                LOG_OUTPUT(L"Page.Loaded event raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPage;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar = safe_cast<xaml_controls::CommandBar^>(rootPage->Content);
            VERIFY_IS_NOT_NULL(commandBar);

            commandBarOpenedEventRegistration.Attach(commandBar, [&]
            {
                LOG_OUTPUT(L"CommandBar.Opened event raised.");
                commandBarOpenedEvent->Set();
            });

            commandBarClosedEventRegistration.Attach(commandBar, [&]
            {
                LOG_OUTPUT(L"CommandBar.Closed event raised.");
                commandBarClosedEvent->Set();
            });

            appBarButton = dynamic_cast<xaml_controls::AppBarButton^>(commandBar->PrimaryCommands->GetAt(4));
            VERIFY_IS_NOT_NULL(appBarButton);

            gotFocusEventRegistration.Attach(appBarButton, [&]
            {
                LOG_OUTPUT(L"AppBarButton.GotFocus event raised.");
                gotFocusEvent->Set();
            });

            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(appBarButton->Flyout);
            VERIFY_IS_NOT_NULL(menuFlyout);

            menuFlyoutOpenedEventRegistration.Attach(menuFlyout, [&]
            {
                LOG_OUTPUT(L"MenuFlyout.Opened event raised.");
                menuFlyoutOpenedEvent->Set();
            });

            menuFlyoutClosedEventRegistration.Attach(menuFlyout, [&]
            {
                LOG_OUTPUT(L"MenuFlyout.Closed event raised.");
                menuFlyoutClosedEvent->Set();
            });

            moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(commandBar, L"MoreButton"));
            VERIFY_IS_NOT_NULL(moreButton);

            LOG_OUTPUT(L"Setting keyboard focus on AppBarButton.");
            appBarButton->Focus(FocusState::Keyboard);
        });
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking AppBarButton to open MenuFlyout.");
        TestServices::KeyboardHelper->Enter();
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            menuFlyoutPosition1 = menuFlyout->Items->GetAt(0)->TransformToVisual(appBarButton)->TransformPoint(wf::Point(0, 0));
            LOG_OUTPUT(L"MenuFlyout position (1): %f, %f", menuFlyoutPosition1.X, menuFlyoutPosition1.Y);
        });

        LOG_OUTPUT(L"Escaping to close MenuFlyout.");
        TestServices::KeyboardHelper->Escape();
        menuFlyoutClosedEvent->WaitForDefault();
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Increasing CommandBar.Margin.Right to move AppBarButton into overflow section.");
            commandBar->Margin = xaml::ThicknessHelper::FromLengths(0, 0, 250, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking MoreButton to open overflow.");
        TestServices::KeyboardHelper->Enter();
        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting keyboard focus on AppBarButton.");
            appBarButton->Focus(FocusState::Keyboard);
        });
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking AppBarButton to open MenuFlyout.");
        TestServices::KeyboardHelper->Enter();
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Escaping to close MenuFlyout.");
        TestServices::KeyboardHelper->Escape();
        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Escaping to close overflow.");
        TestServices::KeyboardHelper->Escape();
        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Decreasing CommandBar.Margin.Right to move AppBarButton out of overflow section.");
            commandBar->Margin = xaml::ThicknessHelper::FromLengths(0, 0, 200, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting keyboard focus on AppBarButton.");
            appBarButton->Focus(FocusState::Keyboard);
        });
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking AppBarButton to open MenuFlyout.");
        TestServices::KeyboardHelper->Enter();
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            menuFlyoutPosition2 = menuFlyout->Items->GetAt(0)->TransformToVisual(appBarButton)->TransformPoint(wf::Point(0, 0));
            LOG_OUTPUT(L"MenuFlyout position (2): %f, %f", menuFlyoutPosition2.X, menuFlyoutPosition2.Y);
        });

        LOG_OUTPUT(L"Escaping to close MenuFlyout.");
        TestServices::KeyboardHelper->Escape();
        menuFlyoutClosedEvent->WaitForDefault();
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verifying MenuFlyout positions before and after display in overflow are identical.");
        VERIFY_ARE_EQUAL(menuFlyoutPosition1.X, menuFlyoutPosition2.X);
        VERIFY_ARE_EQUAL(menuFlyoutPosition1.Y, menuFlyoutPosition2.Y);
    }

    void CommandBarIntegrationTests::VerifyAppBarButtonFlyoutLightDismiss()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 800));

        auto rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetPackageFolder()
            + L"resources\\native\\controls\\commandbar\\CommandBarTest.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);

        auto commandBarOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto commandBarOpenedEvent = std::make_shared<Event>();

        auto commandBarClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto commandBarClosedEvent = std::make_shared<Event>();

        auto menuFlyoutOpenedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyoutOpenedEvent = std::make_shared<Event>();

        auto menuFlyoutClosedEventRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);
        auto menuFlyoutClosedEvent = std::make_shared<Event>();

        auto moreButtonGotFocusEventRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
        auto moreButtonGotFocusEvent = std::make_shared<Event>();

        auto appBarButtonGotFocusEventRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
        auto appBarButtonGotFocusEvent = std::make_shared<Event>();

        xaml_controls::CommandBar^ commandBar = nullptr;
        xaml_controls::AppBarButton^ appBarButton0 = nullptr;
        xaml_controls::AppBarButton^ appBarButton4 = nullptr;
        xaml_controls::MenuFlyout^ menuFlyout = nullptr;
        xaml_controls::Button^ moreButton = nullptr;

        RunOnUIThread([&]()
        {
            loadedEventRegistration.Attach(rootPage, [&]
            {
                LOG_OUTPUT(L"Page.Loaded event raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPage;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            commandBar = safe_cast<xaml_controls::CommandBar^>(rootPage->Content);
            VERIFY_IS_NOT_NULL(commandBar);

            commandBarOpenedEventRegistration.Attach(commandBar, [&]
            {
                LOG_OUTPUT(L"CommandBar.Opened event raised.");
                commandBarOpenedEvent->Set();
            });

            commandBarClosedEventRegistration.Attach(commandBar, [&]
            {
                LOG_OUTPUT(L"CommandBar.Closed event raised.");
                commandBarClosedEvent->Set();
            });

            appBarButton0 = dynamic_cast<xaml_controls::AppBarButton^>(commandBar->PrimaryCommands->GetAt(0));
            VERIFY_IS_NOT_NULL(appBarButton0);

            appBarButton4 = dynamic_cast<xaml_controls::AppBarButton^>(commandBar->PrimaryCommands->GetAt(4));
            VERIFY_IS_NOT_NULL(appBarButton4);

            appBarButtonGotFocusEventRegistration.Attach(appBarButton4, [&]
            {
                LOG_OUTPUT(L"AppBarButton.GotFocus event raised.");
                appBarButtonGotFocusEvent->Set();
            });

            menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(appBarButton4->Flyout);
            VERIFY_IS_NOT_NULL(menuFlyout);

            menuFlyoutOpenedEventRegistration.Attach(menuFlyout, [&]
            {
                LOG_OUTPUT(L"MenuFlyout.Opened event raised.");
                menuFlyoutOpenedEvent->Set();
            });

            menuFlyoutClosedEventRegistration.Attach(menuFlyout, [&]
            {
                LOG_OUTPUT(L"MenuFlyout.Closed event raised.");
                menuFlyoutClosedEvent->Set();
            });

            moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(commandBar, L"MoreButton"));
            VERIFY_IS_NOT_NULL(moreButton);

            moreButtonGotFocusEventRegistration.Attach(moreButton, [&]
            {
                LOG_OUTPUT(L"MoreButton.GotFocus event raised.");
                moreButtonGotFocusEvent->Set();
            });

            LOG_OUTPUT(L"Increasing CommandBar.Margin.Right to move AppBarButton into overflow section.");
            commandBar->Margin = xaml::ThicknessHelper::FromLengths(0, 0, 250, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting keyboard focus on MoreButton.");
            moreButton->Focus(FocusState::Keyboard);
        });
        moreButtonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking MoreButton to open overflow.");
        TestServices::KeyboardHelper->Enter();
        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting keyboard focus on AppBarButton.");
            appBarButton4->Focus(FocusState::Keyboard);
        });
        appBarButtonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking AppBarButton to open MenuFlyout.");
        TestServices::KeyboardHelper->Enter();
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Escaping to close MenuFlyout.");
        TestServices::KeyboardHelper->Escape();
        menuFlyoutClosedEvent->WaitForDefault();
        appBarButtonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Escaping to close overflow.");
        TestServices::KeyboardHelper->Escape();
        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Decreasing CommandBar.Margin.Right to move AppBarButton out of overflow section.");
            commandBar->Margin = xaml::ThicknessHelper::FromLengths(0, 0, 200, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting keyboard focus on AppBarButton.");
            appBarButton4->Focus(FocusState::Keyboard);
        });
        appBarButtonGotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking AppBarButton to open MenuFlyout.");
        TestServices::KeyboardHelper->Enter();
        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping outside of MenuFlyout to dismiss it.");
        TestServices::InputHelper->Tap(appBarButton0);
        menuFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::VerifyAccessKeyBehavior()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;

        RunOnUIThread([&]()
        {
            cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                          AccessKey="M" >
                        <AppBarButton Icon="Add" Label="Add" />
                        <AppBarButton Icon="Back" Label="Back" />
                        <CommandBar.SecondaryCommands>
                            <AppBarButton Icon="Favorite" Label="Favorite" />
                            <AppBarButton Icon="Calculator" Label="Calc" />
                        </CommandBar.SecondaryCommands>
                    </CommandBar>)"));

            auto rootGrid = ref new xaml_controls::Grid();
            rootGrid->Children->Append(cmdBar);
            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Even though we set the AccessKey on the CommandBar, in UIA we expect it to also appear on the More Button.
            // This is because the CommandBar typically does not get focused and so there otherwise would be no way for a 
            // Narrator user to know that the accesskey is available. 
            xaml_controls::Control^ moreButton = GetMoreButton(cmdBar);
            xaml_automation_peers::AutomationPeer^ moreButtonAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(moreButton);
            xaml_automation_peers::AutomationPeer^ cmdBarAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(cmdBar);
            Platform::String^ expectedAccessKeyStr = L"Alt, M";
            VERIFY_ARE_EQUAL(expectedAccessKeyStr, cmdBarAP->GetAccessKey());
            VERIFY_ARE_EQUAL(expectedAccessKeyStr, moreButtonAP->GetAccessKey());

            // Verify that if we change the AccessKey, that is reflected on the AutomationPeers for both the CommandBar
            // and the More Button.
            cmdBar->AccessKey = L"L";
            expectedAccessKeyStr = L"Alt, L";
            VERIFY_ARE_EQUAL(expectedAccessKeyStr, cmdBarAP->GetAccessKey());
            VERIFY_ARE_EQUAL(expectedAccessKeyStr, moreButtonAP->GetAccessKey());
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that invoking the access key opens and closes the CommandBar:
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_l#$u$_l#$u$_alt");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(cmdBar->IsOpen);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_l#$u$_l#$u$_alt");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(cmdBar->IsOpen);
        });
    }

    void CommandBarIntegrationTests::VerifyClosingSubMenuDoesNotClearFocus()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::CommandBar^ commandBar = nullptr;
        xaml_controls::AppBarButton^ button1 = nullptr;
        xaml_controls::AppBarButton^ button2 = nullptr;

        auto commandBarOpenedEvent = std::make_shared<Event>();
        auto commandBarClosedEvent = std::make_shared<Event>();
        auto menuFlyout1OpenedEvent = std::make_shared<Event>();
        auto menuFlyout1ClosedEvent = std::make_shared<Event>();
        auto menuFlyout2OpenedEvent = std::make_shared<Event>();
        auto menuFlyout2ClosedEvent = std::make_shared<Event>();
        auto losingFocusEvent = std::make_shared<Event>();
        auto commandBarOpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto commandBarClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto menuFlyout1OpenedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyout1ClosedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);
        auto menuFlyout2OpenedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyout2ClosedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        wf::EventRegistrationToken losingFocusToken{};

        RunOnUIThread([&]()
        {
            commandBar = ref new xaml_controls::CommandBar();
            commandBar->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            commandBar->VerticalAlignment = xaml::VerticalAlignment::Center;

            button1 = ref new xaml_controls::AppBarButton();
            button1->Label = L"Item 1";
            commandBar->SecondaryCommands->Append(button1);

            button2 = ref new xaml_controls::AppBarButton();
            button2->Label = L"Item 2";
            commandBar->SecondaryCommands->Append(button2);

            auto menuFlyout1 = ref new xaml_controls::MenuFlyout();
            auto item = ref new xaml_controls::MenuFlyoutItem();
            item->Text = L"Sub item 1.1";
            menuFlyout1->Items->Append(item);
            button1->Flyout = menuFlyout1;

            auto menuFlyout2 = ref new xaml_controls::MenuFlyout();
            item = ref new xaml_controls::MenuFlyoutItem();
            item->Text = L"Sub item 2.1";
            menuFlyout2->Items->Append(item);
            button2->Flyout = menuFlyout2;

            commandBarOpenedRegistration.Attach(commandBar, [commandBarOpenedEvent]()
            {
                LOG_OUTPUT(L"CommandBar opened.");
                commandBarOpenedEvent->Set();
            });

            commandBarClosedRegistration.Attach(commandBar, [commandBarClosedEvent]()
            {
                LOG_OUTPUT(L"CommandBar closed.");
                commandBarClosedEvent->Set();
            });

            menuFlyout1OpenedRegistration.Attach(menuFlyout1, [menuFlyout1OpenedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout 1 opened.");
                menuFlyout1OpenedEvent->Set();
            });

            menuFlyout1ClosedRegistration.Attach(menuFlyout1, [menuFlyout1ClosedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout 1 closed.");
                menuFlyout1ClosedEvent->Set();
            });

            menuFlyout2OpenedRegistration.Attach(menuFlyout2, [menuFlyout2OpenedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout 2 opened.");
                menuFlyout2OpenedEvent->Set();
            });

            menuFlyout2ClosedRegistration.Attach(menuFlyout2, [menuFlyout2ClosedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout 2 closed.");
                menuFlyout2ClosedEvent->Set();
            });

            losingFocusToken = xaml_input::FocusManager::LosingFocus += ref new wf::EventHandler<xaml_input::LosingFocusEventArgs^>([losingFocusEvent](Platform::Object^ sender, xaml_input::LosingFocusEventArgs^ args)
            {
                if (args->NewFocusedElement == nullptr)
                {
                    LOG_OUTPUT(L"XAML focus lost.");
                    losingFocusEvent->Set();
                }
            });

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(commandBar);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Opening the CommandBar.");
            commandBar->IsOpen = true;
        });

        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving the mouse over the first item to open its MenuFlyout.");
        TestServices::InputHelper->MoveMouse(button1);

        menuFlyout1OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving the mouse over the second item to open its MenuFlyout.");
        TestServices::InputHelper->MoveMouse(button2);

        menuFlyout1ClosedEvent->WaitForDefault();
        menuFlyout2OpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verifying that XAML did not lose focus.");
        VERIFY_IS_FALSE(losingFocusEvent->HasFired());

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Closing the CommandBar.");
            commandBar->IsOpen = false;
        });

        menuFlyout2ClosedEvent->WaitForDefault();
        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_input::FocusManager::LosingFocus -= losingFocusToken;
        });
    }

    void CommandBarIntegrationTests::VerifySubMenuOpensLeftInRTL()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ commandBar = nullptr;
        xaml_controls::AppBarButton^ button = nullptr;
        xaml_controls::MenuFlyoutItem^ item = nullptr;

        auto commandBarOpenedEvent = std::make_shared<Event>();
        auto commandBarClosedEvent = std::make_shared<Event>();
        auto menuFlyoutOpenedEvent = std::make_shared<Event>();
        auto menuFlyoutClosedEvent = std::make_shared<Event>();
        auto commandBarOpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto commandBarClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto menuFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
        auto menuFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);

        RunOnUIThread([&]()
        {
            commandBar = ref new xaml_controls::CommandBar();
            commandBar->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            commandBar->VerticalAlignment = xaml::VerticalAlignment::Center;

            button = ref new xaml_controls::AppBarButton();
            button->Label = L"Item 1";
            commandBar->SecondaryCommands->Append(button);

            auto menuFlyout = ref new xaml_controls::MenuFlyout();
            item = ref new xaml_controls::MenuFlyoutItem();
            item->Text = L"Sub item 1.1";
            menuFlyout->Items->Append(item);
            button->Flyout = menuFlyout;

            commandBarOpenedRegistration.Attach(commandBar, [commandBarOpenedEvent]()
            {
                LOG_OUTPUT(L"CommandBar opened.");
                commandBarOpenedEvent->Set();
            });

            commandBarClosedRegistration.Attach(commandBar, [commandBarClosedEvent]()
            {
                LOG_OUTPUT(L"CommandBar closed.");
                commandBarClosedEvent->Set();
            });

            menuFlyoutOpenedRegistration.Attach(menuFlyout, [menuFlyoutOpenedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout opened.");
                menuFlyoutOpenedEvent->Set();
            });

            menuFlyoutClosedRegistration.Attach(menuFlyout, [menuFlyoutClosedEvent]()
            {
                LOG_OUTPUT(L"MenuFlyout closed.");
                menuFlyoutClosedEvent->Set();
            });

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->FlowDirection = xaml::FlowDirection::RightToLeft;
            rootPanel->Children->Append(commandBar);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Opening the CommandBar.");
            commandBar->IsOpen = true;
        });

        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving the mouse over the AppBarButton to open its MenuFlyout.");
        TestServices::InputHelper->MoveMouse(button);

        menuFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying that the MenuFlyoutItem is to the left of the AppBarButton.");
            auto buttonBounds = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(button, false);
            auto itemBounds = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(item, false);

            VERIFY_IS_LESS_THAN(itemBounds.X, buttonBounds.X);

            LOG_OUTPUT(L"Closing the CommandBar.");
            commandBar->IsOpen = false;
        });

        menuFlyoutClosedEvent->WaitForDefault();
        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandBarIntegrationTests::VerifySubMenuStaysInViewWhenBothLeftAndRightFailToFit()
    {
        TestCleanupWrapper cleanup;

        float windowWidth = 200;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(windowWidth, 200));

        xaml_controls::CommandBar^ commandBar = nullptr;
        xaml_controls::AppBarButton^ button = nullptr;
        xaml_controls::TextBlock^ item = nullptr;

        auto commandBarOpenedEvent = std::make_shared<Event>();
        auto commandBarClosedEvent = std::make_shared<Event>();
        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto commandBarOpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
        auto commandBarClosedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Closed);
        auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            commandBar = ref new xaml_controls::CommandBar();
            commandBar->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            commandBar->VerticalAlignment = xaml::VerticalAlignment::Top;

            button = ref new xaml_controls::AppBarButton();
            button->Label = L"1";
            commandBar->SecondaryCommands->Append(button);

            auto flyout = ref new xaml_controls::Flyout();
            flyout->ShouldConstrainToRootBounds = true;
            item = ref new xaml_controls::TextBlock();
            item->Text = L"Sub item 1.1";
            flyout->Content = item;
            button->Flyout = flyout;

            commandBarOpenedRegistration.Attach(commandBar, [commandBarOpenedEvent]()
            {
                LOG_OUTPUT(L"CommandBar opened.");
                commandBarOpenedEvent->Set();
            });

            commandBarClosedRegistration.Attach(commandBar, [commandBarClosedEvent]()
            {
                LOG_OUTPUT(L"CommandBar closed.");
                commandBarClosedEvent->Set();
            });

            flyoutOpenedRegistration.Attach(flyout, [flyoutOpenedEvent]()
            {
                LOG_OUTPUT(L"Flyout opened.");
                flyoutOpenedEvent->Set();
            });

            flyoutClosedRegistration.Attach(flyout, [flyoutClosedEvent]()
            {
                LOG_OUTPUT(L"Flyout closed.");
                flyoutClosedEvent->Set();
            });

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);
            rootPanel->Children->Append(commandBar);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Opening the CommandBar.");
            commandBar->IsOpen = true;
        });

        commandBarOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving the mouse over the AppBarButton to open its Flyout.");
        TestServices::InputHelper->MoveMouse(button);

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying that the Flyout opens to the right, but remains in bounds.");
            auto buttonBounds = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(button, false);
            auto itemBounds = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(item, false);

            LOG_OUTPUT(L"(buttonBounds.X, buttonBounds.Y, buttonBounds.Width, buttonBounds.Height) = (%.0f, %.0f, %.0f, %.0f)", buttonBounds.X, buttonBounds.Y, buttonBounds.Width, buttonBounds.Height);
            LOG_OUTPUT(L"(itemBounds.X, itemBounds.Y, itemBounds.Width, itemBounds.Height) = (%.0f, %.0f, %.0f, %.0f)", itemBounds.X, itemBounds.Y, itemBounds.Width, itemBounds.Height);

            VERIFY_IS_GREATER_THAN(itemBounds.X, buttonBounds.X);
            VERIFY_IS_LESS_THAN(itemBounds.X + itemBounds.Height, windowWidth + 1);

            LOG_OUTPUT(L"Closing the CommandBar.");
            commandBar->IsOpen = false;
        });

        flyoutClosedEvent->WaitForDefault();
        commandBarClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::CommandBar
