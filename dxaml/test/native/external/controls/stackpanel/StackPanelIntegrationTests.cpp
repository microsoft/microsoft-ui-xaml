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
#include <TreeHelper.h>

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

    //
    // IInsertionPanel
    //
    // Four 100x100 items (PanelsHelper default content). StackPanel computes insertion indexes from the children's
    // desired sizes along the stacking direction:
    //   item 0: [0,100)  item 1: [100,200)  item 2: [200,300)  item 3: [300,400)
    // A point in the leading half of item k lies before k; a point in the trailing half lies after k. A point exactly at
    // the midpoint of item k is still in its leading half, so it lies before k. Every point beyond the last item,
    // however far past it, lies after the last item, so the item before the point is the last item. The value reported
    // for 'second' at the end of a panel (and for an empty panel) is logged but not asserted: the platform's
    // end-of-panel convention for it is not documented.
    //
    void StackPanelIntegrationTests::VerifyGetInsertionIndexesVertical()
    {
        TestCleanupWrapper cleanup;
        const auto probes = ProbeInsertionIndexes(Orientation::Vertical);

        VERIFY_ARE_EQUAL(-1, probes.leadingHalfOfItem0.first);
        VERIFY_ARE_EQUAL(0, probes.leadingHalfOfItem0.second);
        VERIFY_ARE_EQUAL(0, probes.trailingHalfOfItem0.first);
        VERIFY_ARE_EQUAL(1, probes.trailingHalfOfItem0.second);
        VERIFY_ARE_EQUAL(0, probes.leadingHalfOfItem1.first);
        VERIFY_ARE_EQUAL(1, probes.leadingHalfOfItem1.second);
        VERIFY_ARE_EQUAL(0, probes.midpointOfItem1.first);
        VERIFY_ARE_EQUAL(1, probes.midpointOfItem1.second);
        VERIFY_ARE_EQUAL(1, probes.trailingHalfOfItem1.first);
        VERIFY_ARE_EQUAL(2, probes.trailingHalfOfItem1.second);
        VERIFY_ARE_EQUAL(1, probes.leadingHalfOfItem2.first);
        VERIFY_ARE_EQUAL(2, probes.leadingHalfOfItem2.second);
        VERIFY_ARE_EQUAL(3, probes.justPastTheEnd.first);
        VERIFY_ARE_EQUAL(3, probes.pastTheEnd.first);
        VERIFY_ARE_EQUAL(3, probes.moreThanHalfAnItemPastTheEnd.first);
        VERIFY_ARE_EQUAL(-1, probes.emptyPanel.first);
        VERIFY_IS_TRUE(probes.offThreadCallFailedWithWrongThread);
    }

    void StackPanelIntegrationTests::VerifyGetInsertionIndexesHorizontal()
    {
        TestCleanupWrapper cleanup;
        const auto probes = ProbeInsertionIndexes(Orientation::Horizontal);

        VERIFY_ARE_EQUAL(-1, probes.leadingHalfOfItem0.first);
        VERIFY_ARE_EQUAL(0, probes.leadingHalfOfItem0.second);
        VERIFY_ARE_EQUAL(0, probes.trailingHalfOfItem0.first);
        VERIFY_ARE_EQUAL(1, probes.trailingHalfOfItem0.second);
        VERIFY_ARE_EQUAL(0, probes.leadingHalfOfItem1.first);
        VERIFY_ARE_EQUAL(1, probes.leadingHalfOfItem1.second);
        VERIFY_ARE_EQUAL(0, probes.midpointOfItem1.first);
        VERIFY_ARE_EQUAL(1, probes.midpointOfItem1.second);
        VERIFY_ARE_EQUAL(1, probes.trailingHalfOfItem1.first);
        VERIFY_ARE_EQUAL(2, probes.trailingHalfOfItem1.second);
        VERIFY_ARE_EQUAL(1, probes.leadingHalfOfItem2.first);
        VERIFY_ARE_EQUAL(2, probes.leadingHalfOfItem2.second);
        VERIFY_ARE_EQUAL(3, probes.justPastTheEnd.first);
        VERIFY_ARE_EQUAL(3, probes.pastTheEnd.first);
        VERIFY_ARE_EQUAL(3, probes.moreThanHalfAnItemPastTheEnd.first);
        VERIFY_ARE_EQUAL(-1, probes.emptyPanel.first);
        VERIFY_IS_TRUE(probes.offThreadCallFailedWithWrongThread);
    }

    StackPanelIntegrationTests::InsertionIndexProbes StackPanelIntegrationTests::ProbeInsertionIndexes(xaml_controls::Orientation orientation)
    {
        const int itemCount = 4;
        const bool isVertical = (orientation == Orientation::Vertical);
        auto pointAt = [isVertical](float offset)
        {
            // The cross-axis coordinate does not affect StackPanel insertion; keep it inside the items.
            return isVertical ? wf::Point(50.0f, offset) : wf::Point(offset, 50.0f);
        };

        auto stackPanel = PanelsHelper::AddPanelWithContent<xaml_controls::StackPanel>(PanelsHelper::CreateDefaultPanelContent(itemCount), orientation);
        TestServices::WindowHelper->WaitForIdle();

        InsertionIndexProbes probes = {};
        xaml_controls::IInsertionPanel^ insertionPanel = nullptr;

        RunOnUIThread([&]()
        {
            insertionPanel = safe_cast<xaml_controls::IInsertionPanel^>(stackPanel);
            insertionPanel->GetInsertionIndexes(pointAt(10.0f), &probes.leadingHalfOfItem0.first, &probes.leadingHalfOfItem0.second);
            insertionPanel->GetInsertionIndexes(pointAt(60.0f), &probes.trailingHalfOfItem0.first, &probes.trailingHalfOfItem0.second);
            insertionPanel->GetInsertionIndexes(pointAt(140.0f), &probes.leadingHalfOfItem1.first, &probes.leadingHalfOfItem1.second);
            insertionPanel->GetInsertionIndexes(pointAt(150.0f), &probes.midpointOfItem1.first, &probes.midpointOfItem1.second);
            insertionPanel->GetInsertionIndexes(pointAt(160.0f), &probes.trailingHalfOfItem1.first, &probes.trailingHalfOfItem1.second);
            insertionPanel->GetInsertionIndexes(pointAt(240.0f), &probes.leadingHalfOfItem2.first, &probes.leadingHalfOfItem2.second);
            insertionPanel->GetInsertionIndexes(pointAt(410.0f), &probes.justPastTheEnd.first, &probes.justPastTheEnd.second);
            insertionPanel->GetInsertionIndexes(pointAt(450.0f), &probes.pastTheEnd.first, &probes.pastTheEnd.second);
            insertionPanel->GetInsertionIndexes(pointAt(460.0f), &probes.moreThanHalfAnItemPastTheEnd.first, &probes.moreThanHalfAnItemPastTheEnd.second);

            auto emptyStackPanel = ref new xaml_controls::StackPanel();
            emptyStackPanel->Orientation = orientation;
            safe_cast<xaml_controls::IInsertionPanel^>(emptyStackPanel)->GetInsertionIndexes(pointAt(10.0f), &probes.emptyPanel.first, &probes.emptyPanel.second);
        });

        LOG_OUTPUT(L"Leading half of item 0: first=%d second=%d.", probes.leadingHalfOfItem0.first, probes.leadingHalfOfItem0.second);
        LOG_OUTPUT(L"Trailing half of item 0: first=%d second=%d.", probes.trailingHalfOfItem0.first, probes.trailingHalfOfItem0.second);
        LOG_OUTPUT(L"Leading half of item 1: first=%d second=%d.", probes.leadingHalfOfItem1.first, probes.leadingHalfOfItem1.second);
        LOG_OUTPUT(L"Trailing half of item 1: first=%d second=%d.", probes.trailingHalfOfItem1.first, probes.trailingHalfOfItem1.second);
        LOG_OUTPUT(L"Leading half of item 2: first=%d second=%d.", probes.leadingHalfOfItem2.first, probes.leadingHalfOfItem2.second);
        LOG_OUTPUT(L"Midpoint of item 1: first=%d second=%d.", probes.midpointOfItem1.first, probes.midpointOfItem1.second);
        LOG_OUTPUT(L"Just past the end: first=%d second=%d.", probes.justPastTheEnd.first, probes.justPastTheEnd.second);
        LOG_OUTPUT(L"Past the end: first=%d second=%d.", probes.pastTheEnd.first, probes.pastTheEnd.second);
        LOG_OUTPUT(L"More than half an item past the end: first=%d second=%d.", probes.moreThanHalfAnItemPastTheEnd.first, probes.moreThanHalfAnItemPastTheEnd.second);
        LOG_OUTPUT(L"Empty panel: first=%d second=%d.", probes.emptyPanel.first, probes.emptyPanel.second);

        LOG_OUTPUT(L"Calling GetInsertionIndexes off the UI thread.");
        try
        {
            int first = 0;
            int second = 0;
            insertionPanel->GetInsertionIndexes(pointAt(10.0f), &first, &second);
            LOG_OUTPUT(L"The off-thread call unexpectedly succeeded: first=%d second=%d.", first, second);
        }
        catch (Platform::Exception^ ex)
        {
            LOG_OUTPUT(L"The off-thread call failed with 0x%08x.", ex->HResult);
            probes.offThreadCallFailedWithWrongThread = (ex->HResult == RPC_E_WRONG_THREAD);
        }

        return probes;
    }

    //
    // Property accessors and dependency properties
    //
    static void VerifyThicknessEquals(double left, double top, double right, double bottom, xaml::Thickness actual)
    {
        VERIFY_ARE_EQUAL(left, actual.Left);
        VERIFY_ARE_EQUAL(top, actual.Top);
        VERIFY_ARE_EQUAL(right, actual.Right);
        VERIFY_ARE_EQUAL(bottom, actual.Bottom);
    }

    // COM identity: two references denote the same object exactly when their IUnknown pointers are equal. Handles of
    // different projected types (e.g. SolidColorBrush^ and Brush^) point at different interfaces of the same object.
    static bool IsSameComObject(Platform::Object^ first, Platform::Object^ second)
    {
        if (first == nullptr || second == nullptr)
        {
            return (first == nullptr) && (second == nullptr);
        }

        wrl::ComPtr<IUnknown> firstIdentity;
        wrl::ComPtr<IUnknown> secondIdentity;
        VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(first)->QueryInterface(IID_PPV_ARGS(&firstIdentity)));
        VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(second)->QueryInterface(IID_PPV_ARGS(&secondIdentity)));
        return firstIdentity.Get() == secondIdentity.Get();
    }

    static void VerifyCornerRadiusEquals(double topLeft, double topRight, double bottomRight, double bottomLeft, xaml::CornerRadius actual)
    {
        VERIFY_ARE_EQUAL(topLeft, actual.TopLeft);
        VERIFY_ARE_EQUAL(topRight, actual.TopRight);
        VERIFY_ARE_EQUAL(bottomRight, actual.BottomRight);
        VERIFY_ARE_EQUAL(bottomLeft, actual.BottomLeft);
    }

    void StackPanelIntegrationTests::VerifyPropertiesMatchDependencyProperties()
    {
        TestCleanupWrapper cleanup;
        const auto snapshots = CapturePropertySnapshots();

        LOG_OUTPUT(L"Default values agree between the property getters and the dependency properties.");
        VERIFY_ARE_EQUAL(xaml_controls::BackgroundSizing::InnerBorderEdge, snapshots.defaults.backgroundSizingFromGetter);
        VERIFY_ARE_EQUAL(xaml_controls::BackgroundSizing::InnerBorderEdge, snapshots.defaults.backgroundSizingFromProperty);
        VERIFY_ARE_EQUAL(0.0, snapshots.defaults.spacingFromGetter);
        VERIFY_ARE_EQUAL(0.0, snapshots.defaults.spacingFromProperty);
        VERIFY_ARE_EQUAL(xaml_controls::Orientation::Vertical, snapshots.defaults.orientationFromProperty);
        VERIFY_IS_FALSE(snapshots.defaults.areScrollSnapPointsRegularFromProperty);
        VERIFY_IS_TRUE(snapshots.defaults.borderBrushFromPropertyIsNull);
        VerifyThicknessEquals(0, 0, 0, 0, snapshots.defaults.borderThicknessFromProperty);
        VerifyThicknessEquals(0, 0, 0, 0, snapshots.defaults.paddingFromProperty);
        VerifyCornerRadiusEquals(0, 0, 0, 0, snapshots.defaults.cornerRadiusFromProperty);

        LOG_OUTPUT(L"Values set through the property setters are visible through the dependency properties.");
        VERIFY_ARE_EQUAL(xaml_controls::BackgroundSizing::OuterBorderEdge, snapshots.afterPropertySetters.backgroundSizingFromGetter);
        VERIFY_ARE_EQUAL(xaml_controls::BackgroundSizing::OuterBorderEdge, snapshots.afterPropertySetters.backgroundSizingFromProperty);
        VERIFY_IS_TRUE(snapshots.afterPropertySetters.borderBrushFromGetterIsSetBrush);
        VERIFY_IS_TRUE(snapshots.afterPropertySetters.borderBrushFromPropertyIsSetBrush);
        VerifyThicknessEquals(1, 2, 3, 4, snapshots.afterPropertySetters.borderThicknessFromProperty);
        VerifyThicknessEquals(9, 10, 11, 12, snapshots.afterPropertySetters.paddingFromProperty);
        VerifyCornerRadiusEquals(5, 6, 7, 8, snapshots.afterPropertySetters.cornerRadiusFromProperty);
        VerifyCornerRadiusEquals(5, 6, 7, 8, snapshots.afterPropertySetters.cornerRadiusFromGetter);
        VERIFY_ARE_EQUAL(12.5, snapshots.afterPropertySetters.spacingFromGetter);
        VERIFY_ARE_EQUAL(12.5, snapshots.afterPropertySetters.spacingFromProperty);
        VERIFY_ARE_EQUAL(xaml_controls::Orientation::Horizontal, snapshots.afterPropertySetters.orientationFromProperty);
        VERIFY_IS_TRUE(snapshots.afterPropertySetters.areScrollSnapPointsRegularFromProperty);

        LOG_OUTPUT(L"Values set through the dependency properties are visible through the property getters.");
        VERIFY_ARE_EQUAL(xaml_controls::BackgroundSizing::InnerBorderEdge, snapshots.afterDependencyPropertySetters.backgroundSizingFromGetter);
        VERIFY_ARE_EQUAL(xaml_controls::BackgroundSizing::InnerBorderEdge, snapshots.afterDependencyPropertySetters.backgroundSizingFromProperty);
        VERIFY_ARE_EQUAL(3.0, snapshots.afterDependencyPropertySetters.spacingFromGetter);
        VERIFY_ARE_EQUAL(3.0, snapshots.afterDependencyPropertySetters.spacingFromProperty);
    }

    StackPanelIntegrationTests::PropertySnapshots StackPanelIntegrationTests::CapturePropertySnapshots()
    {
        PropertySnapshots snapshots = {};

        RunOnUIThread([&]()
        {
            auto stackPanel = ref new xaml_controls::StackPanel();
            auto borderBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);

            auto readSnapshot = [&]()
            {
                PropertySnapshot snapshot = {};
                snapshot.backgroundSizingFromGetter = stackPanel->BackgroundSizing;
                snapshot.backgroundSizingFromProperty = safe_cast<xaml_controls::BackgroundSizing>(stackPanel->GetValue(xaml_controls::StackPanel::BackgroundSizingProperty));
                snapshot.spacingFromGetter = stackPanel->Spacing;
                snapshot.spacingFromProperty = safe_cast<double>(stackPanel->GetValue(xaml_controls::StackPanel::SpacingProperty));
                snapshot.orientationFromProperty = safe_cast<xaml_controls::Orientation>(stackPanel->GetValue(xaml_controls::StackPanel::OrientationProperty));
                snapshot.areScrollSnapPointsRegularFromProperty = safe_cast<bool>(stackPanel->GetValue(xaml_controls::StackPanel::AreScrollSnapPointsRegularProperty));
                snapshot.borderBrushFromPropertyIsNull = (stackPanel->GetValue(xaml_controls::StackPanel::BorderBrushProperty) == nullptr);
                snapshot.borderBrushFromGetterIsSetBrush = IsSameComObject(borderBrush, stackPanel->BorderBrush);
                snapshot.borderBrushFromPropertyIsSetBrush = IsSameComObject(borderBrush, stackPanel->GetValue(xaml_controls::StackPanel::BorderBrushProperty));
                snapshot.borderThicknessFromProperty = safe_cast<xaml::Thickness>(stackPanel->GetValue(xaml_controls::StackPanel::BorderThicknessProperty));
                snapshot.paddingFromProperty = safe_cast<xaml::Thickness>(stackPanel->GetValue(xaml_controls::StackPanel::PaddingProperty));
                snapshot.cornerRadiusFromGetter = stackPanel->CornerRadius;
                snapshot.cornerRadiusFromProperty = safe_cast<xaml::CornerRadius>(stackPanel->GetValue(xaml_controls::StackPanel::CornerRadiusProperty));
                return snapshot;
            };

            snapshots.defaults = readSnapshot();

            stackPanel->BackgroundSizing = xaml_controls::BackgroundSizing::OuterBorderEdge;
            stackPanel->BorderBrush = borderBrush;
            stackPanel->BorderThickness = xaml::ThicknessHelper::FromLengths(1, 2, 3, 4);
            stackPanel->CornerRadius = xaml::CornerRadiusHelper::FromRadii(5, 6, 7, 8);
            stackPanel->Padding = xaml::ThicknessHelper::FromLengths(9, 10, 11, 12);
            stackPanel->Spacing = 12.5;
            stackPanel->Orientation = xaml_controls::Orientation::Horizontal;
            stackPanel->AreScrollSnapPointsRegular = true;
            snapshots.afterPropertySetters = readSnapshot();

            stackPanel->SetValue(xaml_controls::StackPanel::BackgroundSizingProperty, xaml_controls::BackgroundSizing::InnerBorderEdge);
            stackPanel->SetValue(xaml_controls::StackPanel::SpacingProperty, 3.0);
            snapshots.afterDependencyPropertySetters = readSnapshot();
        });

        return snapshots;
    }

    //
    // IPaginatedPanel queries made by ListViewBase incremental loading against a StackPanel items host
    //

    // An IAsyncOperation that completes later on the UI thread, the way an incremental data source normally does.
    ref class DeferredLoadMoreItemsOperation sealed : public wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>
    {
    public:
        DeferredLoadMoreItemsOperation()
            : m_status(wf::AsyncStatus::Started)
        {
        }

        virtual property wf::AsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult>^ Completed
        {
            wf::AsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult>^ get()
            {
                return m_completed;
            }

            void set(wf::AsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult>^ value)
            {
                m_completed = value;

                DeferredLoadMoreItemsOperation^ self = this;
                TestServices::WindowHelper->CurrentDispatcher->TryEnqueue(
                    Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
                    ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([self]()
                    {
                        self->Complete();
                    }));
            }
        }

        virtual xaml_data::LoadMoreItemsResult GetResults()
        {
            xaml_data::LoadMoreItemsResult result;
            result.Count = 0;
            return result;
        }

        virtual property unsigned int Id
        {
            unsigned int get()
            {
                return 1;
            }
        }

        virtual property wf::AsyncStatus Status
        {
            wf::AsyncStatus get()
            {
                return m_status;
            }
        }

        virtual property wf::HResult ErrorCode
        {
            wf::HResult get()
            {
                wf::HResult result;
                result.Value = S_OK;
                return result;
            }
        }

        virtual void Cancel()
        {
        }

        virtual void Close()
        {
        }

    private:
        void Complete()
        {
            m_status = wf::AsyncStatus::Completed;
            auto completed = m_completed;
            m_completed = nullptr;
            if (completed)
            {
                completed(this, m_status);
            }
        }

        wf::AsyncStatus m_status;
        wf::AsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult>^ m_completed;
    };

    // A read-only items source that supports incremental loading and records each load request.
    ref class IncrementalLoadingTestSource sealed :
        public wfc::IObservableVector<Platform::Object^>,
        public xaml_data::ISupportIncrementalLoading
    {
    public:
        IncrementalLoadingTestSource(unsigned int itemCount)
            : m_items(ref new Platform::Collections::Vector<Platform::Object^>())
            , m_hasMoreItems(true)
            , m_loadRequestCount(0)
            , m_lastRequestedCount(0)
        {
            for (unsigned int i = 0; i < itemCount; ++i)
            {
                m_items->Append(i);
            }
        }

        property unsigned int LoadRequestCount
        {
            unsigned int get()
            {
                return m_loadRequestCount;
            }
        }

        property unsigned int LastRequestedCount
        {
            unsigned int get()
            {
                return m_lastRequestedCount;
            }
        }

        // ISupportIncrementalLoading
        virtual property bool HasMoreItems
        {
            bool get()
            {
                return m_hasMoreItems;
            }
        }

        virtual wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>^ LoadMoreItemsAsync(unsigned int count)
        {
            ++m_loadRequestCount;
            m_lastRequestedCount = count;

            // Report the end of the data so that the list's re-evaluation after completion does not chain more loads.
            m_hasMoreItems = false;
            return ref new DeferredLoadMoreItemsOperation();
        }

        // IObservableVector. The ListView only reads its ItemsSource in these tests.
        virtual event wfc::VectorChangedEventHandler<Platform::Object^>^ VectorChanged;

        virtual wfc::IIterator<Platform::Object^>^ First()
        {
            return m_items->First();
        }

        virtual property unsigned int Size
        {
            unsigned int get()
            {
                return m_items->Size;
            }
        }

        virtual Platform::Object^ GetAt(unsigned int index)
        {
            return m_items->GetAt(index);
        }

        virtual wfc::IVectorView<Platform::Object^>^ GetView()
        {
            return m_items->GetView();
        }

        virtual bool IndexOf(Platform::Object^ value, unsigned int* index)
        {
            return m_items->IndexOf(value, index);
        }

        virtual unsigned int GetMany(unsigned int startIndex, Platform::WriteOnlyArray<Platform::Object^>^ items)
        {
            return m_items->GetMany(startIndex, items);
        }

        virtual void SetAt(unsigned int, Platform::Object^)
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual void InsertAt(unsigned int, Platform::Object^)
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual void RemoveAt(unsigned int)
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual void Append(Platform::Object^)
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual void RemoveAtEnd()
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual void Clear()
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual void ReplaceAll(const Platform::Array<Platform::Object^>^)
        {
            throw ref new Platform::NotImplementedException();
        }

    private:
        Platform::Collections::Vector<Platform::Object^>^ m_items;
        bool m_hasMoreItems;
        unsigned int m_loadRequestCount;
        unsigned int m_lastRequestedCount;
    };

    static const unsigned int s_incrementalItemCount = 40;
    static const double s_incrementalItemExtent = 50.0;
    static const double s_incrementalDataFetchSize = 2.0;
    static const double s_incrementalLoadingThreshold = 1.0;
    static const double s_incrementalViewportExtent = 200.0;
    static const double s_incrementalCrossAxisExtent = 400.0;

    // A ListView whose items host is a StackPanel with fixed-size containers along the stacking direction. The list is
    // 200 along the stacking direction and 400 across it, so the two viewport dimensions differ.
    static xaml_controls::ListView^ CreateListViewWithStackPanelItemsHost(xaml_controls::Orientation orientation, bool useEdgeTrigger, double itemExtent)
    {
        const bool isVertical = (orientation == Orientation::Vertical);
        const std::wstring stackingExtent = std::to_wstring(s_incrementalViewportExtent);
        const std::wstring crossAxisExtent = std::to_wstring(s_incrementalCrossAxisExtent);

        std::wstring markup =
            L"<ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L" DataFetchSize='2' IncrementalLoadingThreshold='1'";
        markup += L" Width='" + (isVertical ? crossAxisExtent : stackingExtent) + L"'";
        markup += L" Height='" + (isVertical ? stackingExtent : crossAxisExtent) + L"'";
        markup += useEdgeTrigger ? L" IncrementalLoadingTrigger='Edge'" : L" IncrementalLoadingTrigger='None'";
        if (!isVertical)
        {
            markup +=
                L" ScrollViewer.HorizontalScrollMode='Enabled' ScrollViewer.HorizontalScrollBarVisibility='Auto'"
                L" ScrollViewer.VerticalScrollMode='Disabled' ScrollViewer.VerticalScrollBarVisibility='Disabled'";
        }
        markup +=
            L">"
            L"<ListView.ItemContainerTransitions><TransitionCollection/></ListView.ItemContainerTransitions>"
            L"<ListView.ItemsPanel><ItemsPanelTemplate><StackPanel Orientation='";
        markup += isVertical ? L"Vertical" : L"Horizontal";
        markup +=
            L"'/></ItemsPanelTemplate></ListView.ItemsPanel>"
            L"<ListView.ItemContainerStyle><Style TargetType='ListViewItem'>"
            L"<Setter Property='MinWidth' Value='0'/>"
            L"<Setter Property='MinHeight' Value='0'/>"
            L"<Setter Property='Padding' Value='0'/>"
            L"<Setter Property='Margin' Value='0'/>"
            L"<Setter Property='UseLayoutRounding' Value='False'/>"
            L"<Setter Property='";
        markup += isVertical ? L"Height" : L"Width";
        markup += L"' Value='" + std::to_wstring(itemExtent) + L"'/>";
        markup +=
            L"</Style></ListView.ItemContainerStyle>"
            L"</ListView>";

        return safe_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(ref new Platform::String(markup.c_str())));
    }

    // Confirms the preconditions the incremental loading oracles rely on: the items host is a StackPanel, every item
    // container has the fixed extent along the stacking direction, and the viewport is 200 along the stacking direction
    // and larger across it.
    static xaml_controls::ScrollViewer^ VerifyStackPanelItemsHostWithFixedExtents(xaml_controls::ListView^ listView, bool isVertical, unsigned int itemCount, double itemExtent)
    {
        VERIFY_IS_NOT_NULL(dynamic_cast<xaml_controls::StackPanel^>(listView->ItemsPanelRoot));

        for (unsigned int i = 0; i < itemCount; ++i)
        {
            auto container = safe_cast<xaml::UIElement^>(listView->ContainerFromIndex(static_cast<int>(i)));
            const double extent = static_cast<double>(isVertical ? container->DesiredSize.Height : container->DesiredSize.Width);
            VERIFY_ARE_EQUAL(itemExtent, extent);
        }

        auto scrollViewer = dynamic_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(listView, L"ScrollViewer"));
        VERIFY_IS_NOT_NULL(scrollViewer);

        const double viewportExtent = isVertical ? scrollViewer->ViewportHeight : scrollViewer->ViewportWidth;
        const double crossAxisViewportExtent = isVertical ? scrollViewer->ViewportWidth : scrollViewer->ViewportHeight;
        LOG_OUTPUT(L"Viewport extent along the stacking direction: %f, across it: %f.", viewportExtent, crossAxisViewportExtent);
        VERIFY_ARE_EQUAL(s_incrementalViewportExtent, viewportExtent);
        VERIFY_IS_GREATER_THAN(crossAxisViewportExtent, viewportExtent);
        return scrollViewer;
    }

    // The number of items ListViewBase requests: DataFetchSize pages, where a page is the viewport divided by the
    // average item extent along the stacking direction.
    static unsigned int GetExpectedLoadCount(double viewportExtent)
    {
        return static_cast<unsigned int>(s_incrementalDataFetchSize * (viewportExtent / s_incrementalItemExtent));
    }

    static void ChangeViewAndWait(xaml_controls::ScrollViewer^ scrollViewer, bool isVertical, double offset)
    {
        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    if (!args->IsIntermediate)
                    {
                        viewChangedEvent->Set();
                    }
                }));

            LOG_OUTPUT(L"Scrolling to offset %f.", offset);
            const bool accepted = isVertical ?
                scrollViewer->ChangeView(nullptr, offset, nullptr, true) :
                scrollViewer->ChangeView(offset, nullptr, nullptr, true);
            VERIFY_IS_TRUE(accepted);
        });

        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void StackPanelIntegrationTests::VerifyLoadMoreItemsAsyncUsesItemsPerPageVertical()
    {
        TestCleanupWrapper cleanup;
        const auto observation = ObserveExplicitLoadMoreItems(Orientation::Vertical, s_incrementalItemExtent);

        // Nothing is loaded until LoadMoreItemsAsync is called (IncrementalLoadingTrigger=None), then exactly one
        // request for DataFetchSize pages of (viewport height / item height) items is made: 2 x (200 / 50). The
        // viewport is twice as wide as it is high, so a page measured across the stacking direction would differ.
        VERIFY_ARE_EQUAL(0u, observation.requestsBeforeAction);
        VERIFY_ARE_EQUAL(1u, observation.requestsAfterAction);
        VERIFY_ARE_EQUAL(observation.expectedRequestedCount, observation.lastRequestedCount);
        VERIFY_ARE_EQUAL(8u, observation.lastRequestedCount);

        // Items no larger than one pixel along the stacking direction do not define a page, so the list falls back
        // to requesting a single item.
        const auto onePixelItemsObservation = ObserveExplicitLoadMoreItems(Orientation::Vertical, 1.0);
        VERIFY_ARE_EQUAL(0u, onePixelItemsObservation.requestsBeforeAction);
        VERIFY_ARE_EQUAL(1u, onePixelItemsObservation.requestsAfterAction);
        VERIFY_ARE_EQUAL(1u, onePixelItemsObservation.lastRequestedCount);
    }

    void StackPanelIntegrationTests::VerifyLoadMoreItemsAsyncUsesItemsPerPageHorizontal()
    {
        TestCleanupWrapper cleanup;
        const auto observation = ObserveExplicitLoadMoreItems(Orientation::Horizontal, s_incrementalItemExtent);

        // Nothing is loaded until LoadMoreItemsAsync is called (IncrementalLoadingTrigger=None), then exactly one
        // request for DataFetchSize pages of (viewport width / item width) items is made: 2 x (200 / 50). The
        // viewport is twice as high as it is wide, so a page measured across the stacking direction would differ.
        VERIFY_ARE_EQUAL(0u, observation.requestsBeforeAction);
        VERIFY_ARE_EQUAL(1u, observation.requestsAfterAction);
        VERIFY_ARE_EQUAL(observation.expectedRequestedCount, observation.lastRequestedCount);
        VERIFY_ARE_EQUAL(8u, observation.lastRequestedCount);

        // Items no larger than one pixel along the stacking direction do not define a page, so the list falls back
        // to requesting a single item.
        const auto onePixelItemsObservation = ObserveExplicitLoadMoreItems(Orientation::Horizontal, 1.0);
        VERIFY_ARE_EQUAL(0u, onePixelItemsObservation.requestsBeforeAction);
        VERIFY_ARE_EQUAL(1u, onePixelItemsObservation.requestsAfterAction);
        VERIFY_ARE_EQUAL(1u, onePixelItemsObservation.lastRequestedCount);
    }

    StackPanelIntegrationTests::IncrementalLoadObservation StackPanelIntegrationTests::ObserveExplicitLoadMoreItems(xaml_controls::Orientation orientation, double itemExtent)
    {
        const bool isVertical = (orientation == Orientation::Vertical);
        xaml_controls::ListView^ listView = nullptr;
        IncrementalLoadingTestSource^ source = nullptr;
        IncrementalLoadObservation observation = {};

        RunOnUIThread([&]()
        {
            listView = CreateListViewWithStackPanelItemsHost(orientation, false /* useEdgeTrigger */, itemExtent);
            source = ref new IncrementalLoadingTestSource(s_incrementalItemCount);
            listView->ItemsSource = source;
            TestServices::WindowHelper->WindowContent = listView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto scrollViewer = VerifyStackPanelItemsHostWithFixedExtents(listView, isVertical, s_incrementalItemCount, itemExtent);
            const double viewportExtent = isVertical ? scrollViewer->ViewportHeight : scrollViewer->ViewportWidth;
            observation.expectedRequestedCount = GetExpectedLoadCount(viewportExtent);
            observation.requestsBeforeAction = source->LoadRequestCount;

            LOG_OUTPUT(L"Calling ListView.LoadMoreItemsAsync with %f-pixel items.", itemExtent);
            auto loadOperation = listView->LoadMoreItemsAsync();
            VERIFY_IS_NOT_NULL(loadOperation);

            observation.requestsAfterAction = source->LoadRequestCount;
            observation.lastRequestedCount = source->LastRequestedCount;
        });

        // Let the data source complete its deferred operation on the UI thread.
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Requests before: %u, after: %u; requested %u items (%u for a full page of %f-pixel items).",
            observation.requestsBeforeAction, observation.requestsAfterAction, observation.lastRequestedCount, observation.expectedRequestedCount, s_incrementalItemExtent);
        return observation;
    }

    //
    // With IncrementalLoadingTrigger=Edge, the list requests more items once fewer than (IncrementalLoadingThreshold + 1)
    // whole pages of items remain after the last item in the viewport. An item is in the viewport while its leading
    // edge is at or before the viewport's trailing edge. Each scenario scrolls:
    //   - by one item near the start: far from the end, nothing is requested;
    //   - so the first item that would put the list within the threshold starts half an item past the viewport's
    //     trailing edge: that item is not in the viewport yet, so nothing is requested;
    //   - so that item's leading edge is exactly at the viewport's trailing edge: it is now in the viewport, so exactly
    //     one request for DataFetchSize pages is made;
    //   - to the end: the source reported that it has no more items, so nothing more is requested.
    // The scenario runs with two collection sizes, so the item at the viewport's trailing edge is reached both while
    // the panel searches its children and as the search's final candidate.
    //
    void StackPanelIntegrationTests::VerifyEdgeIncrementalLoadingNearEndVertical()
    {
        TestCleanupWrapper cleanup;

        const auto observation = ObserveEdgeIncrementalLoading(Orientation::Vertical, 32);
        VERIFY_ARE_EQUAL(0u, observation.requestsBeforeAction);
        VERIFY_ARE_EQUAL(0u, observation.requestsAfterNearStartScroll);
        VERIFY_ARE_EQUAL(0u, observation.requestsAfterHalfAnItemBeforeThresholdScroll);
        VERIFY_ARE_EQUAL(1u, observation.requestsAfterThresholdScroll);
        VERIFY_ARE_EQUAL(1u, observation.requestsAfterAction);
        VERIFY_ARE_EQUAL(observation.expectedRequestedCount, observation.lastRequestedCount);

        const auto otherSizeObservation = ObserveEdgeIncrementalLoading(Orientation::Vertical, 30);
        VERIFY_ARE_EQUAL(0u, otherSizeObservation.requestsBeforeAction);
        VERIFY_ARE_EQUAL(0u, otherSizeObservation.requestsAfterNearStartScroll);
        VERIFY_ARE_EQUAL(0u, otherSizeObservation.requestsAfterHalfAnItemBeforeThresholdScroll);
        VERIFY_ARE_EQUAL(1u, otherSizeObservation.requestsAfterThresholdScroll);
        VERIFY_ARE_EQUAL(1u, otherSizeObservation.requestsAfterAction);
        VERIFY_ARE_EQUAL(otherSizeObservation.expectedRequestedCount, otherSizeObservation.lastRequestedCount);
    }

    void StackPanelIntegrationTests::VerifyEdgeIncrementalLoadingNearEndHorizontal()
    {
        TestCleanupWrapper cleanup;

        const auto observation = ObserveEdgeIncrementalLoading(Orientation::Horizontal, 32);
        VERIFY_ARE_EQUAL(0u, observation.requestsBeforeAction);
        VERIFY_ARE_EQUAL(0u, observation.requestsAfterNearStartScroll);
        VERIFY_ARE_EQUAL(0u, observation.requestsAfterHalfAnItemBeforeThresholdScroll);
        VERIFY_ARE_EQUAL(1u, observation.requestsAfterThresholdScroll);
        VERIFY_ARE_EQUAL(1u, observation.requestsAfterAction);
        VERIFY_ARE_EQUAL(observation.expectedRequestedCount, observation.lastRequestedCount);

        const auto otherSizeObservation = ObserveEdgeIncrementalLoading(Orientation::Horizontal, 30);
        VERIFY_ARE_EQUAL(0u, otherSizeObservation.requestsBeforeAction);
        VERIFY_ARE_EQUAL(0u, otherSizeObservation.requestsAfterNearStartScroll);
        VERIFY_ARE_EQUAL(0u, otherSizeObservation.requestsAfterHalfAnItemBeforeThresholdScroll);
        VERIFY_ARE_EQUAL(1u, otherSizeObservation.requestsAfterThresholdScroll);
        VERIFY_ARE_EQUAL(1u, otherSizeObservation.requestsAfterAction);
        VERIFY_ARE_EQUAL(otherSizeObservation.expectedRequestedCount, otherSizeObservation.lastRequestedCount);
    }

    StackPanelIntegrationTests::IncrementalLoadObservation StackPanelIntegrationTests::ObserveEdgeIncrementalLoading(xaml_controls::Orientation orientation, unsigned int itemCount)
    {
        const bool isVertical = (orientation == Orientation::Vertical);
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        IncrementalLoadingTestSource^ source = nullptr;
        IncrementalLoadObservation observation = {};
        double scrollableExtent = 0.0;
        double thresholdOffset = 0.0;

        RunOnUIThread([&]()
        {
            listView = CreateListViewWithStackPanelItemsHost(orientation, true /* useEdgeTrigger */, s_incrementalItemExtent);
            source = ref new IncrementalLoadingTestSource(itemCount);
            listView->ItemsSource = source;
            TestServices::WindowHelper->WindowContent = listView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewer = VerifyStackPanelItemsHostWithFixedExtents(listView, isVertical, itemCount, s_incrementalItemExtent);
            const double viewportExtent = isVertical ? scrollViewer->ViewportHeight : scrollViewer->ViewportWidth;
            scrollableExtent = isVertical ? scrollViewer->ScrollableHeight : scrollViewer->ScrollableWidth;

            // Near the start, many more pages than the one-page threshold remain past the viewport.
            VERIFY_IS_GREATER_THAN(scrollableExtent, 4 * viewportExtent);

            // The first item that puts the list within the threshold when it is the last item in the viewport.
            const unsigned int itemsPerPage = static_cast<unsigned int>(viewportExtent / s_incrementalItemExtent);
            const unsigned int thresholdItemIndex = itemCount - static_cast<unsigned int>((s_incrementalLoadingThreshold + 1) * itemsPerPage);
            thresholdOffset = thresholdItemIndex * s_incrementalItemExtent - viewportExtent;
            LOG_OUTPUT(L"%u items, viewport extent: %f, scrollable extent: %f; item %u starts at the viewport's trailing edge at offset %f.",
                itemCount, viewportExtent, scrollableExtent, thresholdItemIndex, thresholdOffset);
            VERIFY_IS_GREATER_THAN(thresholdOffset, 2 * s_incrementalItemExtent);
            VERIFY_IS_LESS_THAN(thresholdOffset, scrollableExtent);

            observation.expectedRequestedCount = GetExpectedLoadCount(viewportExtent);
            observation.requestsBeforeAction = source->LoadRequestCount;
        });

        LOG_OUTPUT(L"Scrolling by one item near the start.");
        ChangeViewAndWait(scrollViewer, isVertical, s_incrementalItemExtent);
        RunOnUIThread([&]()
        {
            observation.requestsAfterNearStartScroll = source->LoadRequestCount;
        });

        LOG_OUTPUT(L"Scrolling so the threshold item starts half an item past the viewport.");
        ChangeViewAndWait(scrollViewer, isVertical, thresholdOffset - s_incrementalItemExtent / 2);
        RunOnUIThread([&]()
        {
            observation.requestsAfterHalfAnItemBeforeThresholdScroll = source->LoadRequestCount;
        });

        LOG_OUTPUT(L"Scrolling so the threshold item starts exactly at the viewport's trailing edge.");
        ChangeViewAndWait(scrollViewer, isVertical, thresholdOffset);
        RunOnUIThread([&]()
        {
            observation.requestsAfterThresholdScroll = source->LoadRequestCount;
            observation.lastRequestedCount = source->LastRequestedCount;
        });

        // Let the data source complete its deferred operation on the UI thread.
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Scrolling to the end.");
        ChangeViewAndWait(scrollViewer, isVertical, scrollableExtent);
        RunOnUIThread([&]()
        {
            observation.requestsAfterAction = source->LoadRequestCount;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Requests initially: %u, near the start: %u, half an item before the threshold: %u, at the threshold: %u, at the end: %u; requested %u items, expected %u.",
            observation.requestsBeforeAction, observation.requestsAfterNearStartScroll, observation.requestsAfterHalfAnItemBeforeThresholdScroll,
            observation.requestsAfterThresholdScroll, observation.requestsAfterAction, observation.lastRequestedCount, observation.expectedRequestedCount);
        return observation;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::StackPanel
