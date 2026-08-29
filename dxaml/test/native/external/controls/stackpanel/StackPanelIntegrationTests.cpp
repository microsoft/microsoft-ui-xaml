// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StackPanelIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <PanelsHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <Patterns\InvokePatternHandler.h>

#include <ChangeDPI.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Automation;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace StackPanel {

    bool StackPanelIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool StackPanelIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool StackPanelIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void StackPanelIntegrationTests::CanStackItemsHorizontally()
    {
        TestCleanupWrapper cleanup;
        auto stackPanel = PanelsHelper::AddPanelWithContent<xaml_controls::StackPanel>(PanelsHelper::CreateDefaultPanelContent(s_itemCount), Orientation::Horizontal);
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            // y-coordinate of each rectangle's origin is 100 because each rectangle is 100 pixels tall, the
            // StackPanel is 300 pixels tall and by default the rectangles are center-aligned vertically
            expectedPositions->Append(wf::Point(100.0f * i, 100.0f));
        }

        PanelsHelper::VerifyItemPositions(stackPanel, expectedPositions);
    }

    void StackPanelIntegrationTests::CanStackItemsVertically()
    {
        TestCleanupWrapper cleanup;
        auto stackPanel = PanelsHelper::AddPanelWithContent<xaml_controls::StackPanel>(PanelsHelper::CreateDefaultPanelContent(s_itemCount), Orientation::Vertical);
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            // x-coordinate of each rectangle's origin is 100 because each rectangle is 100 pixels wide, the
            // StackPanel is 300 pixels wide and by default the rectangles are center-aligned horizontally
            expectedPositions->Append(wf::Point(100.0f, 100.0f * i));
        }

        PanelsHelper::VerifyItemPositions(stackPanel, expectedPositions);
    }

    void StackPanelIntegrationTests::CanStackVariableSizedItemsHorizontally()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<UIElement^>^ itemsVector = nullptr;

        RunOnUIThread([&]()
        {
            itemsVector = ref new Platform::Collections::Vector<UIElement^>();

            itemsVector->Append(CreateRectangle(100, 50));
            itemsVector->Append(CreateRectangle(300, 200, false));  // collapsed
            itemsVector->Append(CreateRectangle(25, 200));
            itemsVector->Append(CreateRectangle(75, 100));
        });

        auto stackPanel = PanelsHelper::AddPanelWithContent<xaml_controls::StackPanel>(itemsVector, Orientation::Horizontal);
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        // Child rectangles are center-aligned vertically by default
        expectedPositions->Append(wf::Point(0.0f, 125.0f));
        expectedPositions->Append(wf::Point(0.0f, 0.0f));
        expectedPositions->Append(wf::Point(100.0f, 50.0f));
        expectedPositions->Append(wf::Point(125.0f, 100.0f));

        PanelsHelper::VerifyItemPositions(stackPanel, expectedPositions);
    }

    void StackPanelIntegrationTests::CanStackVariableSizedItemsVertically()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<UIElement^>^ itemsVector = nullptr;

        RunOnUIThread([&]()
        {
            itemsVector = ref new Platform::Collections::Vector<UIElement^>();

            itemsVector->Append(CreateRectangle(50, 100));
            itemsVector->Append(CreateRectangle(300, 200, false));  // collapsed
            itemsVector->Append(CreateRectangle(200, 25));
            itemsVector->Append(CreateRectangle(100, 75));
        });

        auto stackPanel = PanelsHelper::AddPanelWithContent<xaml_controls::StackPanel>(itemsVector, Orientation::Vertical);
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        // Child rectangles are center-aligned horizontally by default
        expectedPositions->Append(wf::Point(125.0f, 0.0f));
        expectedPositions->Append(wf::Point(0.0f, 0.0f));
        expectedPositions->Append(wf::Point(50.0f, 100.0f));
        expectedPositions->Append(wf::Point(100.0f, 125.0f));

        PanelsHelper::VerifyItemPositions(stackPanel, expectedPositions);
    }

    void StackPanelIntegrationTests::CanChangeOrientation()
    {
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        Platform::Collections::Vector<UIElement^>^ itemsVector = nullptr;

        // Build up our vectors of expected rectangle positions
        auto expectedVerticalOrientationPositions = ref new Platform::Collections::Vector<wf::Point>();
        {
            // Child rectangles are center-aligned horizontally by default
            expectedVerticalOrientationPositions->Append(wf::Point(125.0f, 0.0f));
            expectedVerticalOrientationPositions->Append(wf::Point(50.0f, 100.0f));
            expectedVerticalOrientationPositions->Append(wf::Point(100.0f, 125.0f));
        }
        auto expectedHorizontalOrientationPositions = ref new Platform::Collections::Vector<wf::Point>();
        {
            // Child rectangles are center-aligned vertically by default
            expectedHorizontalOrientationPositions->Append(wf::Point(0.0f, 100.0f));
            expectedHorizontalOrientationPositions->Append(wf::Point(50.0f, 137.5f));
            expectedHorizontalOrientationPositions->Append(wf::Point(250.0f, 112.5f));
        }

        RunOnUIThread([&]()
        {
            itemsVector = ref new Platform::Collections::Vector<UIElement^>();

            itemsVector->Append(CreateRectangle(50, 100));
            itemsVector->Append(CreateRectangle(200, 25));
            itemsVector->Append(CreateRectangle(100, 75));
        });

        // Start with the StackPanel vertically oriented
        auto stackPanel = PanelsHelper::AddPanelWithContent<xaml_controls::StackPanel>(itemsVector, Orientation::Vertical);

        PanelsHelper::VerifyItemPositions(stackPanel, expectedVerticalOrientationPositions);
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Vertical");

        LOG_OUTPUT(L"Changing orientation to horizontal.");

        // Now make the StackPanel horizontally oriented
        RunOnUIThread([&]()
        {
            stackPanel->Orientation = Orientation::Horizontal;
        });
        TestServices::WindowHelper->WaitForIdle();

        PanelsHelper::VerifyItemPositions(stackPanel, expectedHorizontalOrientationPositions);
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Horizontal");

        LOG_OUTPUT(L"Changing orientation to vertical.");

        // Switch back to vertical orientation
        RunOnUIThread([&]()
        {
            stackPanel->Orientation = Orientation::Vertical;
        });
        TestServices::WindowHelper->WaitForIdle();

        PanelsHelper::VerifyItemPositions(stackPanel, expectedVerticalOrientationPositions);
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Vertical");
    }

    void StackPanelIntegrationTests::VerifyDesiredSize_AutoLayout()
    {
        TestCleanupWrapper cleanup;

        auto stackPanel = VerifyDesiredSize_Setup();
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 200.0f, 200.0f);

        LOG_OUTPUT(L"Changing orientation to horizontal.");

        // Now make the StackPanel horizontally oriented
        RunOnUIThread([&]()
        {
            stackPanel->Orientation = Orientation::Horizontal;
        });
        TestServices::WindowHelper->WaitForIdle();
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 350.0f, 100.0f);
    }

    void StackPanelIntegrationTests::VerifyDesiredSize_MinWidthHeight()
    {
        TestCleanupWrapper cleanup;

        auto stackPanel = VerifyDesiredSize_Setup();
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 200.0f, 200.0f);

        // Set MinWidth/MinHeight on StackPanel
        RunOnUIThread([&]()
        {
            stackPanel->MinWidth = 250;
            stackPanel->MinHeight = 150;
        });
        TestServices::WindowHelper->WaitForIdle();
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 250.0f, 200.0f);

        LOG_OUTPUT(L"Changing orientation to horizontal.");

        // Now make the StackPanel horizontally oriented
        RunOnUIThread([&]()
        {
            stackPanel->Orientation = Orientation::Horizontal;
        });
        TestServices::WindowHelper->WaitForIdle();
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 350.0f, 150.0f);
    }

    void StackPanelIntegrationTests::VerifyDesiredSize_MaxWidthHeight()
    {
        TestCleanupWrapper cleanup;

        auto stackPanel = VerifyDesiredSize_Setup();
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 200.0f, 200.0f);

        // Set MinWidth/MinHeight on StackPanel
        RunOnUIThread([&]()
        {
            stackPanel->MaxWidth = 175;
            stackPanel->MaxHeight = 50;
        });
        TestServices::WindowHelper->WaitForIdle();
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 175.0f, 50.0f);

        LOG_OUTPUT(L"Changing orientation to horizontal.");

        // Now make the StackPanel horizontally oriented
        RunOnUIThread([&]()
        {
            stackPanel->Orientation = Orientation::Horizontal;
        });
        TestServices::WindowHelper->WaitForIdle();
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 175.0f, 50.0f);
    }

    void StackPanelIntegrationTests::ValidateSpacing()
    {
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Grid^ root = nullptr;
        xaml_controls::StackPanel^ stackPanel = nullptr;

        RunOnUIThread([&]()
        {
            stackPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel"
                    L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"            Background='Yellow'"
                    L"            Spacing='10'"
                    L"            HorizontalAlignment='Center'"
                    L"            VerticalAlignment='Center'"
                    L"            Orientation='Vertical'>"
                    L"  <Border Opacity='0.5' Height='50' Width='50' Background='Red' />"
                    L"  <Border Opacity='0.5' Height='50' Width='50' Background='Blue' />"
                    L"  <Border Opacity='0.5' Height='50' Width='50' Background='Green' />"
                    L"  <Border Opacity='0.5' Height='50' Width='50' Background='Black' Visibility='Collapsed'/>"
                    L"  <Border Opacity='0.5' Height='50' Width='50' Background='Red' />"
                    L"  <Border Opacity='0.5' Height='50' Width='50' Background='Blue' />"
                    L"  <Border Opacity='0.5' Height='50' Width='50' Background='Green' />"
                    L"</StackPanel>"));

            root = ref new xaml_controls::Grid;
            root->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Purple);
            root->Children->Append(stackPanel);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        RunOnUIThread([&]()
        {
            stackPanel->Spacing = -10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

        RunOnUIThread([&]()
        {
            stackPanel->Spacing = 10;
            stackPanel->Orientation = Orientation::Horizontal;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");

        RunOnUIThread([&]()
        {
            stackPanel->Spacing = -10;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "4");
    }

    void StackPanelIntegrationTests::VerifyBorderChrome()
    {
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Verify that basic layout is performed correctly.
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    VerticalAlignment='Top' HorizontalAlignment='Left'"
                L"    Background='Green' BorderThickness='20' BorderBrush='Yellow' CornerRadius='5' Padding='10'>"
                L"    <Rectangle Margin='5' Width='90' Height='40' Fill='Red'/>"
                L"    <Rectangle Margin='5' Width='90' Height='40' Fill='Red'/>"
                L"    <Rectangle Margin='5' Width='90' Height='40' Fill='Red'/>"
                L"    <Rectangle Margin='5' Width='90' Height='40' Fill='Red'/>"
                L"    <Rectangle Margin='5' Width='90' Height='40' Fill='Red'/>"
                L"</StackPanel>"));
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    xaml_controls::StackPanel^ StackPanelIntegrationTests::VerifyDesiredSize_Setup()
    {
        Platform::Collections::Vector<UIElement^>^ itemsVector = nullptr;

        RunOnUIThread([&]()
        {
            itemsVector = ref new Platform::Collections::Vector<UIElement^>();

            itemsVector->Append(CreateRectangle(50, 100));
            itemsVector->Append(CreateRectangle(300, 200, false));  // collapsed
            itemsVector->Append(CreateRectangle(200, 25));
            itemsVector->Append(CreateRectangle(100, 75));
        });

        auto stackPanel = PanelsHelper::AddPanelWithContent<xaml_controls::StackPanel>(itemsVector, Orientation::Vertical);
        PanelsHelper::VerifyPanelDesiredSize(stackPanel, 300.0f, 300.0f);

        LOG_OUTPUT(L"Setting StackPanel to auto layout.");

        // Set StackPanel to auto layout
        RunOnUIThread([&]()
        {
            stackPanel->ClearValue(xaml_controls::StackPanel::WidthProperty);
            stackPanel->ClearValue(xaml_controls::StackPanel::HeightProperty);
        });
        TestServices::WindowHelper->WaitForIdle();

        return stackPanel;
    }

    xaml_shapes::Rectangle^ StackPanelIntegrationTests::CreateRectangle(int width, int height, bool visible)
    {
        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        rectangle->Width = width;
        rectangle->Height = height;

        if (visible)
        {
            rectangle->Visibility = Visibility::Visible;
        }
        else
        {
            rectangle->Visibility = Visibility::Collapsed;
        }

        return rectangle;
    }

    void StackPanelIntegrationTests::VerifySnapPoints()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Validating horizontal SnapPoints.");
        VerifySnapPointsOrientation(Orientation::Horizontal, 50, 25);
        LOG_OUTPUT(L"Validating vertical SnapPoints.");
        VerifySnapPointsOrientation(Orientation::Vertical, 50, 25);
    }

    void StackPanelIntegrationTests::VerifySnapPointsOrientation(Orientation orientation, int elementWidth, int elementHeight)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ stackPanel = nullptr;
        int elementMeasure = orientation == Orientation::Horizontal ? elementWidth : elementHeight;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel();
            stackPanel->AreScrollSnapPointsRegular = true;
            stackPanel->Orientation = orientation;
            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Attach SnapPointsChangedEvent.");

        auto stackPanelHorizontalSnapPointsChangedEvent = std::make_shared<Event>();
        auto horizontalSnapPointsChangedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, HorizontalSnapPointsChanged);
        horizontalSnapPointsChangedRegistration.Attach(stackPanel, ref new wf::EventHandler<Platform::Object^>([stackPanelHorizontalSnapPointsChangedEvent](Platform::Object^, Platform::Object^)
        {
            stackPanelHorizontalSnapPointsChangedEvent->Set();
        }));

        auto stackPanelVerticalSnapPointsChangedEvent = std::make_shared<Event>();
        auto verticalSnapPointsChangedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, VerticalSnapPointsChanged);
        verticalSnapPointsChangedRegistration.Attach(stackPanel, ref new wf::EventHandler<Platform::Object^>([stackPanelVerticalSnapPointsChangedEvent](Platform::Object^, Platform::Object^)
        {
            stackPanelVerticalSnapPointsChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            stackPanel->Children->Append(CreateRectangle(elementWidth, elementHeight));
            stackPanel->Children->Append(CreateRectangle(elementWidth, elementHeight));
        });

        LOG_OUTPUT(L"Wait for RegularSnapPointsChangedEvent activation.");

        if (orientation == Orientation::Horizontal)
        {
            stackPanelHorizontalSnapPointsChangedEvent->WaitForDefault();
        }
        else
        {
            stackPanelVerticalSnapPointsChangedEvent->WaitForDefault();
        }

        RunOnUIThread([&]()
        {
            float offset = 0;
            auto snapRegular = stackPanel->GetRegularSnapPoints(orientation, xaml_primitives::SnapPointsAlignment::Near, &offset);

            // If regular snap points is enabled, the distance between them should be equal to the width or height depending on the orientation
            VERIFY_ARE_EQUAL(snapRegular, elementMeasure);

            stackPanel->AreScrollSnapPointsRegular = false;
            auto snapIrregularNear = stackPanel->GetIrregularSnapPoints(orientation, xaml_primitives::SnapPointsAlignment::Near);
            auto snapIrregularCenter = stackPanel->GetIrregularSnapPoints(orientation, xaml_primitives::SnapPointsAlignment::Center);
            auto snapIrregularFar = stackPanel->GetIrregularSnapPoints(orientation, xaml_primitives::SnapPointsAlignment::Far);

            VERIFY_ARE_EQUAL(snapIrregularNear->Size, 2u);
            VERIFY_ARE_EQUAL(snapIrregularCenter->Size, 2u);
            VERIFY_ARE_EQUAL(snapIrregularFar->Size, 2u);

            // Verify irregular snap points position for the three variants
            // For SnapPointsAlignment::Near snap points start at the beginning of each element eg: Two elements with width or height 50 will have snap points with value 0 and 50
            // For SnapPointsAlignment::Center snap points start at the center of each element eg: Two elements with width or height 50 will have snap points with value 25 and 75
            // For SnapPointsAlignment::Far snap points start at the end of each element eg: Two elements with width or height 50 will have snap points with value 50 and 100
            for (int i = 0; i < 2; i++)
            {
                VERIFY_ARE_EQUAL(snapIrregularNear->GetAt(i), elementMeasure*i);
                VERIFY_ARE_EQUAL(snapIrregularCenter->GetAt(i), elementMeasure*0.5f + elementMeasure*i);
                VERIFY_ARE_EQUAL(snapIrregularFar->GetAt(i), elementMeasure*(i + 1));
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Reset SnapPointsChangedEvent.");

        stackPanelHorizontalSnapPointsChangedEvent->Reset();
        stackPanelVerticalSnapPointsChangedEvent->Reset();

        RunOnUIThread([&]()
        {
            stackPanel->Children->Append(CreateRectangle(elementWidth, elementHeight));
        });

        LOG_OUTPUT(L"Wait for IrregularSnapPointsChangedEvent activation.");

        if (orientation == Orientation::Horizontal)
        {
            stackPanelHorizontalSnapPointsChangedEvent->WaitForDefault();
        }
        else
        {
            stackPanelVerticalSnapPointsChangedEvent->WaitForDefault();
        }

        RunOnUIThread([&]()
        {
            auto snapIrregularNear = stackPanel->GetIrregularSnapPoints(orientation, xaml_primitives::SnapPointsAlignment::Near);
            auto snapIrregularCenter = stackPanel->GetIrregularSnapPoints(orientation, xaml_primitives::SnapPointsAlignment::Center);
            auto snapIrregularFar = stackPanel->GetIrregularSnapPoints(orientation, xaml_primitives::SnapPointsAlignment::Far);
            VERIFY_ARE_EQUAL(snapIrregularNear->Size, 3u);
            VERIFY_ARE_EQUAL(snapIrregularCenter->Size, 3u);
            VERIFY_ARE_EQUAL(snapIrregularFar->Size, 3u);

            for (int i = 0; i < 3; i++)
            {
                VERIFY_ARE_EQUAL(snapIrregularNear->GetAt(i), elementMeasure*i);
                VERIFY_ARE_EQUAL(snapIrregularCenter->GetAt(i), elementMeasure*0.5f + elementMeasure*i);
                VERIFY_ARE_EQUAL(snapIrregularFar->GetAt(i), elementMeasure*(i + 1));
            }
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StackPanelIntegrationTests::ValidateReorderListViewWithStackPanel()
    {
        // Leak: PVLStaggerFunction is leaked, resulting in 75KB???of leaked memory
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        TestCleanupWrapper cleanup;

        xaml::FrameworkElement^ itemAsFE = nullptr;
        xaml_controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <ListView x:Name='listView' Width='300' Height='200' CanReorderItems='True' AllowDrop='True'>"
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <StackPanel/>"
                L"            </ItemsPanelTemplate>"
                L"        </ListView.ItemsPanel>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            auto itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();
            VERIFY_IS_NOT_NULL(itemsSource);

            // populate the items
            for (int i = 0; i < 10; ++i)
            {
                listView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView->SelectedIndex = 0;

            // get the first item so we can reorder it
            itemAsFE = safe_cast<FrameworkElement^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(itemAsFE);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Drag the first item
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemAsFE, 0 /* relX */, 100 /* relY */, 0.1 /* velocityFactor */, 1000 /* holdTime */);
        TestServices::WindowHelper->WaitForIdle();

        // this applies only to Desktop runs
        if (TestServices::Utilities->IsDesktop)
        {
            // Workaround for a known issue
            TestServices::WindowHelper->RestoreForegroundWindow();
        }

        RunOnUIThread([&]()
        {
            // verify that the SelectedIndex changed to something greater than 0
            VERIFY_IS_GREATER_THAN(listView->SelectedIndex, 0);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StackPanelIntegrationTests::VerifyContentClipping()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_controls::StackPanel^ stackPanel = nullptr;

        xaml_shapes::Rectangle^ rectangle1 = nullptr;
        xaml_shapes::Rectangle^ rectangle2 = nullptr;
        xaml_shapes::Rectangle^ rectangle3 = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <StackPanel x:Name='stackPanel' Width='100' Height='200' Background='Purple' >"
                L"        <Rectangle x:Name='rectangle1' MinWidth='200' Width='400' Height='12' Fill='Orange' />"
                L"        <Rectangle x:Name='rectangle2' MinWidth='200' Width='400' Height='12' Fill='Orange' />"
                L"        <Rectangle x:Name='rectangle3' MinWidth='200' Width='400' Height='12' Fill='Orange' />"
                L"    </StackPanel>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            stackPanel = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"stackPanel"));
            VERIFY_IS_NOT_NULL(stackPanel);

            rectangle1 = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectangle1"));
            VERIFY_IS_NOT_NULL(rectangle1);

            rectangle2 = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectangle2"));
            VERIFY_IS_NOT_NULL(rectangle2);

            rectangle3 = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectangle3"));
            VERIFY_IS_NOT_NULL(rectangle3);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            stackPanel->Children->RemoveAt(1);
            stackPanel->Children->InsertAt(0, rectangle2);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void StackPanelIntegrationTests::ValidateBoundingRectangle()
    {
        if (XamlOneCoreTransforms::IsEnabled())
        {
            LOG_OUTPUT(L"This API is not used in OneCoreTransforms mode, skipping test");
            return;
        }

        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);

        // We should test this with a scaling factor, but that is currently broken.
        // Once that is resolved we should set a 2x scale here to ensure that the UIA bounding rect is getting scaled
        // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
        TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, 1.0f);

        xaml_controls::StackPanel^ stackPanel;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel();
            stackPanel->HorizontalAlignment = HorizontalAlignment::Left;
            stackPanel->VerticalAlignment = VerticalAlignment::Top;
            stackPanel->Width = 100;
            stackPanel->Height = 100;
            stackPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);

            TestServices::WindowHelper->WindowContent = stackPanel;

            xaml_automation::AutomationProperties::SetName(stackPanel, ref new Platform::String(L"stackPanel"));
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> stackPanelElement;
            wrl::ComPtr<IUIAutomation> automation;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"stackPanel";

            auto clientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            clientManager->GetAutomation(&automation);
            clientManager->GetCurrentUIAutomationElement(&stackPanelElement);

            VERIFY_IS_NOT_NULL(stackPanelElement);

            RECT rect;
            VERIFY_SUCCEEDED(stackPanelElement->get_CurrentBoundingRectangle(&rect));

            LOG_OUTPUT(L"get_CurrentBoundingRectangle retrieved RECT of [%d,%d,%d,%d]", rect.left, rect.top, rect.right, rect.bottom);
            // The returned rectangle is in screen coordinates which will make the test susceptible to changes in window position/title-bar size.
            // Just validate the width/height as that's the main purpose of this test
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            VERIFY_ARE_EQUAL(width, 100);
            VERIFY_ARE_EQUAL(height, 100);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::StackPanel
