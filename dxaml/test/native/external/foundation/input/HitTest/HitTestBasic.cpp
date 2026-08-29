// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HitTestBasic.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "collection.h"
#include <WUCRenderingScopeGuard.h>

using namespace std;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Markup;
using namespace ::Windows::Foundation::Collections;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

Platform::String^ HitTestBasic::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\HitTest\\";
}

bool HitTestBasic::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool HitTestBasic::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool HitTestBasic::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void HitTestBasic::WindowSetup(UIElement^ root)
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();
}

void HitTestBasic::VerifyTappedPoints(UIElement^ root, UIElement^ tappedElement, std::vector<wf::Point> points)
{
    const int defaultMissedOffset = 15; // Shift tapped points by this many pixels to check for misses.
    const bool defaultVerifyUsingTaps = true; // Verify using both VisualTreeHelper and Taps.

    VerifyTappedPoints(root, tappedElement, points, defaultMissedOffset, defaultVerifyUsingTaps);
}

void HitTestBasic::VerifyTappedPointsNoWindowSetup(
    // In UAP mode we pass in null for the subtree UIElement so it's implicitly understood to be the RootVisual and
    // so we can hit contents of parentless popups. In XamlIslandRoots mode we have to specify a subtree UIElement so
    // Xaml knows what island we're talking about.
    bool specifySubtreeRootElement,
    UIElement^ tappedElement,
    std::vector<wf::Point> points)
{
    const int defaultMissedOffset = 15; // Shift tapped points by this many pixels to check for misses.
    const bool defaultVerifyUsingTaps = true; // Verify using both VisualTreeHelper and Taps.

    UIElement^ rootElement = nullptr;
    if (specifySubtreeRootElement)
    {
        RunOnUIThread([&]()
        {
            auto xamlRoot = tappedElement->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                rootElement = xamlRoot->Content;
            }
        });
    }

    VerifyTappedPoints(rootElement, tappedElement, points, defaultMissedOffset, defaultMissedOffset, defaultVerifyUsingTaps, false /*doWindowSetup*/);
}

void HitTestBasic::VerifyTappedPoints(UIElement^ root, UIElement^ tappedElement, std::vector<wf::Point> points, int missedOffset, bool verifyUsingTaps)
{
    VerifyTappedPoints(root, tappedElement, points, missedOffset, missedOffset, verifyUsingTaps, true /*doWindowSetup*/);
}

// Takes a tree root, an element to tap, and a set of points and attempts to tap the element at those points.
// The test passes if all points result in the "tapped" flag being set.
// Note: The target points provided are not exact, just "close" - exact Outer to Inner points are tested in Transform3DHitTestUnitTests.cpp.
void HitTestBasic::VerifyTappedPoints(UIElement^ root, UIElement^ tappedElement, std::vector<wf::Point> points, int missedOffsetX, int missedOffsetY, bool verifyUsingTaps, bool doWindowSetup)
{
    auto tappedElementEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
    auto tappedElementRegistration = CreateSafeEventRegistration(UIElement, Tapped);

    if (doWindowSetup)
    {
        HitTestBasic::WindowSetup(root);
    }

    RunOnUIThread([&]()
    {
        if (verifyUsingTaps)
        {
            LOG_OUTPUT(L"Set up tapped handler");

            tappedElementRegistration.Attach(
                tappedElement,
                ref new xaml_input::TappedEventHandler(
                    [tappedElementEvent](Platform::Object^ sender, xaml_input::TappedRoutedEventArgs^ args)
                    {
                        LOG_OUTPUT(L"tappedElement was tapped");
                        tappedElementEvent->Set();
                    }));
        }
        else
        {
            LOG_OUTPUT(L"!! Note: Is not using taps to verify.");
        }
    });

    TestServices::WindowHelper->WaitForIdle();

    int numPoints = static_cast<int>(points.size());
    UINT doubleClickTime = ::GetDoubleClickTime();
    LOG_OUTPUT(L"> Verifying inner points, should hit...");
    for (int i = 0; i < numPoints; i++)
    {
        if (verifyUsingTaps)
        {
            LOG_OUTPUT(L"  > Expecting hit: Tap point {%f, %f}", points[i].X, points[i].Y);
            TestServices::InputHelper->Tap(points[i]);

            tappedElementEvent->WaitForDefault();
            VERIFY_IS_TRUE(tappedElementEvent->HasFired());
            tappedElementEvent->Reset();

            // I hate sleeps, but we need to ensure that we don't tap so fast that the gesture recognizer
            // thinks it is a double tap.  If this is an issue, it may be possible to figure out what
            // the maximum distance is to qualify for a double tap and only delay if we are inside that
            // distance.
            ::Sleep(doubleClickTime);

            TestServices::WindowHelper->WaitForIdle();  // For any animations kicked off by the tap
        }

        LOG_OUTPUT(L"  > Expecting hit: Verify point {%f, %f} using VisualTreeHelper", points[i].X, points[i].Y);
        VERIFY_IS_TRUE(HitTestBasic::VerifyPointContainsElementWithVisualTreeHelper(root, tappedElement, points[i]));
    }

    LOG_OUTPUT(L"> Verifying outer points, should miss...");
    for (int i = 0; i < numPoints; i++)
    {
        // Assumes no bottom-right point tested. If a bottom right point is provided, the point translation is expected to result in a hit.
        points[i].X -= missedOffsetX;
        points[i].Y -= missedOffsetY;

        if (verifyUsingTaps)
        {
            LOG_OUTPUT(L"  > Expecting miss: Tap point {%f, %f}", points[i].X, points[i].Y);
            TestServices::InputHelper->Tap(points[i]);

            tappedElementEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(tappedElementEvent->HasFired());

            // I hate sleeps, but we need to ensure that we don't tap so fast that the gesture recognizer
            // thinks it is a double tap.  If this is an issue, it may be possible to figure out what
            // the maximum distance is to qualify for a double tap and only delay if we are inside that
            // distance.
            ::Sleep(doubleClickTime);

            TestServices::WindowHelper->WaitForIdle();  // For any animations kicked off by the tap
        }

        LOG_OUTPUT(L"  > Expecting miss: Verify point {%f, %f} using VisualTreeHelper", points[i].X, points[i].Y);
        VERIFY_IS_FALSE(HitTestBasic::VerifyPointContainsElementWithVisualTreeHelper(root, tappedElement, points[i]));
    }
}

bool HitTestBasic::VerifyPointContainsElementWithVisualTreeHelper(UIElement^ source, UIElement^ target, wf::Point point)
{
    bool containsElement = false;
    RunOnUIThread([&]()
    {
        IIterable<UIElement^>^ hitElements;
        hitElements = VisualTreeHelper::FindElementsInHostCoordinates(point, source);

        for_each (
            begin(hitElements),
            end(hitElements),
            [&](UIElement^ value)
            {
                if (value == target)
                {
                    containsElement = true;
                }
            });
    });
    TestServices::WindowHelper->WaitForIdle();
    return containsElement;
}

void HitTestBasic::StackPanelHitTestRegression()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndStackPanel.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    StackPanel^ stackPanel = nullptr;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

    RunOnUIThread([&]()
    {
        stackPanel = safe_cast<StackPanel^>(grid->FindName(L"ChildStackPanel"));
        VERIFY_IS_NOT_NULL(stackPanel);

        LOG_OUTPUT(L"Set WindowContent");
        TestServices::WindowHelper->WindowContent = grid;
    });
    TestServices::WindowHelper->WaitForIdle();

    wf::Point point = wf::Point(51.0f, 51.0f);
    LOG_OUTPUT(L"Verify point using VisualTreeHelper: Hit");
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(grid, stackPanel, point));
}

void HitTestBasic::ScrollViewerInsidePopupTestRegression()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridWithPopupWithScrollviewer.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    ScrollViewer^ scrollViewer = nullptr;
    Button^ flyoutButton = nullptr;
    Button^ targetButton = nullptr;

    bool tapped = false;
    auto tappedElementEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
    auto tappedElementRegistration = CreateSafeEventRegistration(UIElement, Tapped);

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

    RunOnUIThread([&]()
    {
        scrollViewer = safe_cast<ScrollViewer^>(grid->FindName(L"PopupScrollViewer"));
        VERIFY_IS_NOT_NULL(scrollViewer);

        flyoutButton = safe_cast<Button^>(grid->FindName(L"FlyoutButton"));
        VERIFY_IS_NOT_NULL(flyoutButton);

        targetButton = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(targetButton);

        LOG_OUTPUT(L"Set WindowContent");
        TestServices::WindowHelper->WindowContent = grid;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Set up tapped handler");

        tappedElementRegistration.Attach(
            targetButton,
            ref new xaml_input::TappedEventHandler(
                [tappedElementEvent,&tapped](Platform::Object^ sender, xaml_input::TappedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"tappedElement was tapped");
                    tapped = true;
                    tappedElementEvent->Set();
                }));
    });

    RunOnUIThread([&]()
    {
        flyoutButton->Flyout->ShowAt(flyoutButton);
    });
    TestServices::WindowHelper->WaitForIdle();

    wf::Point point = wf::Point(25.0f, 60.0f); // This point is over the textblock.

    LOG_OUTPUT(L"Initial tap should miss, as the button is not below the textblock");
    TestServices::InputHelper->Tap(point);
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_IS_FALSE(tapped);
    tappedElementEvent->Reset();
    tapped = false;

    LOG_OUTPUT(L"Scroll the button, so it is below the textblock");
    RunOnUIThread([&]()
    {
        scrollViewer->ChangeView(nullptr, 200.0, nullptr);
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tap again. It should still miss because button is not in view.");
    TestServices::InputHelper->Tap(point);
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_IS_FALSE(tapped);
    tappedElementEvent->Reset();
    tapped = false;

    RunOnUIThread([&]()
    {
        flyoutButton->Flyout->Hide();
    });
}

void HitTestBasic::HitTestLayoutClip1Internal()
{
    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"HitTestLayoutClip1.xaml"));
    VERIFY_IS_NOT_NULL(canvas);
    xaml_shapes::Rectangle^ rectangle = nullptr;

    RunOnUIThread([&]()
    {
        rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"myRectangle"));
        VERIFY_IS_NOT_NULL(rectangle);

        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(101.0f, 101.0f)); // Top left
        VerifyTappedPoints(canvas, rectangle, points);
    }

    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(149.0f, 149.0f)); // Bottom right
        VerifyTappedPoints(canvas, rectangle, points, -15, true);
    }
}

void HitTestBasic::HitTestLayoutClip1WUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestLayoutClip1Internal();
}

void HitTestBasic::HitTestLayoutClip2Internal()
{
    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"HitTestLayoutClip2.xaml"));
    VERIFY_IS_NOT_NULL(canvas);
    xaml_shapes::Rectangle^ rectangle = nullptr;

    RunOnUIThread([&]()
    {
        rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"myRectangle"));
        VERIFY_IS_NOT_NULL(rectangle);

        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(131.0f, 131.0f)); // Top left
        VerifyTappedPoints(canvas, rectangle, points);
    }

    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(149.0f, 149.0f)); // Bottom right
        VerifyTappedPoints(canvas, rectangle, points, -15, true);
    }
}

void HitTestBasic::HitTestLayoutClip2WUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestLayoutClip2Internal();
}

void HitTestBasic::HitTestLayoutClip3Internal()
{
    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"HitTestLayoutClip3.xaml"));
    VERIFY_IS_NOT_NULL(canvas);
    xaml_shapes::Rectangle^ rectangle = nullptr;

    RunOnUIThread([&]()
    {
        rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"myRectangle"));
        VERIFY_IS_NOT_NULL(rectangle);

        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    // In non-ContainerVisual mode, a RenderTransform DOES transform the LayoutClip
    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(126.0f, 126.0f)); // Top left
        VerifyTappedPoints(canvas, rectangle, points);
    }

    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(174.0f, 174.0f)); // Bottom right
        VerifyTappedPoints(canvas, rectangle, points, -15, true);
    }
}

void HitTestBasic::HitTestLayoutClip3WUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestLayoutClip3Internal();
}

void HitTestBasic::HitTestLayoutClip4WUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"HitTestLayoutClip4.xaml"));
    VERIFY_IS_NOT_NULL(canvas);
    xaml_controls::StackPanel^ sp = nullptr;

    RunOnUIThread([&]()
    {
        sp = safe_cast< xaml_controls::StackPanel^>(canvas->FindName(L"myStackPanel"));
        VERIFY_IS_NOT_NULL(sp);

        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    // In ContainerVisuals mode, a RenderTransform does NOT transform the LayoutClip for Panels
    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(126.0f, 126.0f)); // Top left
        VerifyTappedPoints(canvas, sp, points);
    }

    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(149.0f, 149.0f)); // Bottom right
        VerifyTappedPoints(canvas, sp, points, -15, true);
    }
}

void HitTestBasic::HitTestRoundedCornerClipWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    Canvas^ root;
    StackPanel^ tapMe;
    Border^ border;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        border = ref new Border();
        border->Width = 200;
        border->Height = 200;

        CornerRadius roundedCorners;
        roundedCorners.TopLeft = 100;
        roundedCorners.TopRight = 100;
        roundedCorners.BottomLeft = 100;
        roundedCorners.BottomRight = 100;
        border->CornerRadius = roundedCorners;

        tapMe = ref new StackPanel();
        tapMe->Width = 200;
        tapMe->Height = 200;
        tapMe->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0, 0xff));

        root->Children->Append(border);
        border->Child = tapMe;
        wh->WindowContent = root;

        pointerPressedRegistration.Attach(
            tapMe,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"PointerPressed event received.");
            pointerPressedEvent->Set();
        }));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"Tap in upper-left, should miss");
    TestServices::InputHelper->Tap(tapMe, 0.02f, 0.02f);
    wh->WaitForIdle();
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());

    LOG_OUTPUT(L"Tap in upper-right, should miss");
    TestServices::InputHelper->Tap(tapMe, 0.98f, 0.02f);
    wh->WaitForIdle();
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());

    LOG_OUTPUT(L"Tap in lower-left, should miss");
    TestServices::InputHelper->Tap(tapMe, 0.02f, 0.98f);
    wh->WaitForIdle();
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());

    LOG_OUTPUT(L"Tap in lower-right, should miss");
    TestServices::InputHelper->Tap(tapMe, 0.98f, 0.98f);
    wh->WaitForIdle();
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());

    LOG_OUTPUT(L"Tap in center, should hit");
    TestServices::InputHelper->Tap(tapMe);
    pointerPressedEvent->WaitForDefault();

    LOG_OUTPUT(L"Verify VisualTreeHelper hit-testing");
    VERIFY_IS_FALSE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(4.0f, 4.0f)));
    VERIFY_IS_FALSE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(196.0f, 4.0f)));
    VERIFY_IS_FALSE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(4.0f, 196.0f)));
    VERIFY_IS_FALSE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(196.0f, 196.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(100.0f, 100.0f)));

    LOG_OUTPUT(L"Removing rounded corners");
    RunOnUIThread([&]()
    {
        CornerRadius roundedCorners;
        roundedCorners.TopLeft = 0;
        roundedCorners.TopRight = 0;
        roundedCorners.BottomLeft = 0;
        roundedCorners.BottomRight = 0;
        border->CornerRadius = roundedCorners;
    });

    LOG_OUTPUT(L"Tap in upper-left, should hit");
    TestServices::InputHelper->Tap(tapMe, 0.02f, 0.02f);
    pointerPressedEvent->WaitForDefault();

    LOG_OUTPUT(L"Tap in upper-right, should hit");
    TestServices::InputHelper->Tap(tapMe, 0.98f, 0.02f);
    pointerPressedEvent->WaitForDefault();

    LOG_OUTPUT(L"Tap in lower-left, should hit");
    TestServices::InputHelper->Tap(tapMe, 0.02f, 0.98f);
    pointerPressedEvent->WaitForDefault();

    LOG_OUTPUT(L"Tap in lower-right, should hit");
    TestServices::InputHelper->Tap(tapMe, 0.98f, 0.98f);
    pointerPressedEvent->WaitForDefault();

    LOG_OUTPUT(L"Tap in center, should hit");
    TestServices::InputHelper->Tap(tapMe);
    pointerPressedEvent->WaitForDefault();

    LOG_OUTPUT(L"Verify VisualTreeHelper hit-testing");
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(4.0f, 4.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(196.0f, 4.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(4.0f, 196.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(196.0f, 196.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, tapMe, wf::Point(100.0f, 100.0f)));
}

void HitTestBasic::HitTestBackgroundInsetWithinBorderWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    Grid^ root;
    Border^ border;

    RunOnUIThread([&]()
    {
        root = ref new Grid();
        root->VerticalAlignment = VerticalAlignment::Top;
        root->HorizontalAlignment = HorizontalAlignment::Left;
        root->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        border = ref new Border();
        border->Width = 200;
        border->Height = 200;
        border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));
        border->BackgroundSizing = BackgroundSizing::InnerBorderEdge;
        border->BorderThickness = xaml::ThicknessHelper::FromUniformLength(20);

        root->Children->Append(border);
        wh->WindowContent = root;

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"PointerPressed event received.");
            pointerPressedEvent->Set();
        }));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"Tap in upper-left, should miss");
    TestServices::InputHelper->Tap(border, 0.02f, 0.02f);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    wh->WaitForIdle();

    LOG_OUTPUT(L"Tap in upper-right, should miss");
    TestServices::InputHelper->Tap(border, 0.98f, 0.02f);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    wh->WaitForIdle();

    LOG_OUTPUT(L"Tap in lower-left, should miss");
    TestServices::InputHelper->Tap(border, 0.02f, 0.98f);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    wh->WaitForIdle();

    LOG_OUTPUT(L"Tap in lower-right, should miss");
    TestServices::InputHelper->Tap(border, 0.98f, 0.98f);
    pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
    VERIFY_IS_FALSE(pointerPressedEvent->HasFired());
    wh->WaitForIdle();

    LOG_OUTPUT(L"Tap in center, should hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());

    LOG_OUTPUT(L"Verify VisualTreeHelper hit-testing");
    VERIFY_IS_FALSE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(4.0f, 4.0f)));
    VERIFY_IS_FALSE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(196.0f, 4.0f)));
    VERIFY_IS_FALSE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(4.0f, 196.0f)));
    VERIFY_IS_FALSE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(196.0f, 196.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(100.0f, 100.0f)));
}

void HitTestBasic::HitTestBackgroundExtendingUnderBorderWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    Grid^ root;
    Border^ border;

    RunOnUIThread([&]()
    {
        root = ref new Grid();
        root->VerticalAlignment = VerticalAlignment::Top;
        root->HorizontalAlignment = HorizontalAlignment::Left;
        root->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        border = ref new Border();
        border->Width = 200;
        border->Height = 200;
        border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));
        border->BackgroundSizing = BackgroundSizing::OuterBorderEdge;
        border->BorderThickness = xaml::ThicknessHelper::FromUniformLength(20);

        root->Children->Append(border);
        wh->WindowContent = root;

        pointerPressedRegistration.Attach(
            border,
            ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"PointerPressed event received.");
            pointerPressedEvent->Set();
        }));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"Tap in upper-left, should hit");
    TestServices::InputHelper->Tap(border, 0.02f, 0.02f);
    pointerPressedEvent->WaitForDefault();
    VERIFY_ARE_EQUAL(pointerPressedEvent->TimesFired(), 1);

    LOG_OUTPUT(L"Tap in upper-right, should hit");
    TestServices::InputHelper->Tap(border, 0.98f, 0.02f);
    pointerPressedEvent->WaitForDefault();
    VERIFY_ARE_EQUAL(pointerPressedEvent->TimesFired(), 2);

    LOG_OUTPUT(L"Tap in lower-left, should hit");
    TestServices::InputHelper->Tap(border, 0.02f, 0.98f);
    pointerPressedEvent->WaitForDefault();
    VERIFY_ARE_EQUAL(pointerPressedEvent->TimesFired(), 3);

    LOG_OUTPUT(L"Tap in lower-right, should hit");
    TestServices::InputHelper->Tap(border, 0.98f, 0.98f);
    pointerPressedEvent->WaitForDefault();
    VERIFY_ARE_EQUAL(pointerPressedEvent->TimesFired(), 4);

    LOG_OUTPUT(L"Tap in center, should hit");
    TestServices::InputHelper->Tap(border);
    pointerPressedEvent->WaitForDefault();
    VERIFY_ARE_EQUAL(pointerPressedEvent->TimesFired(), 5);

    LOG_OUTPUT(L"Verify VisualTreeHelper hit-testing");
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(4.0f, 4.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(196.0f, 4.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(4.0f, 196.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(196.0f, 196.0f)));
    VERIFY_IS_TRUE(VerifyPointContainsElementWithVisualTreeHelper(root, border, wf::Point(100.0f, 100.0f)));
}

void HitTestBasic::HitTestInfiniteOffsetChildWUCFull()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    Border^ tapHere;
    RunOnUIThread([&]()
    {
        Border^ border = ref new Border();
        border->Width = 50;
        border->Height = 50;
        border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
        Canvas::SetLeft(border, std::numeric_limits<double>::infinity());
        Canvas::SetTop(border, std::numeric_limits<double>::infinity());

        Canvas^ canvas = ref new Canvas();
        canvas->Width = 50;
        canvas->Height = 50;
        canvas->HorizontalAlignment = HorizontalAlignment::Left;
        canvas->VerticalAlignment = VerticalAlignment::Top;
        canvas->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0, 0xff));
        canvas->Children->Append(border);

        StackPanel^ stackPanel = ref new StackPanel();
        stackPanel->Margin = xaml::ThicknessHelper::FromUniformLength(50); // Will transform child bounds by (50,50)
        stackPanel->HorizontalAlignment = HorizontalAlignment::Left;
        stackPanel->VerticalAlignment = VerticalAlignment::Top;
        stackPanel->Children->Append(canvas);

        tapHere = ref new Border();
        tapHere->Margin = xaml::ThicknessHelper::FromUniformLength(50);
        tapHere->Width = 50;
        tapHere->Height = 50;
        tapHere->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));
        tapHere->HorizontalAlignment = HorizontalAlignment::Left;
        tapHere->VerticalAlignment = VerticalAlignment::Top;
        tapHere->IsHitTestVisible = false;

        pointerPressedRegistration.Attach(
            stackPanel,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"  > Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        Grid^ root = ref new Grid();
        root->Children->Append(tapHere);
        root->Children->Append(stackPanel);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Clicking - expect hit");
    TestServices::InputHelper->Tap(tapHere);
    pointerPressedEvent->WaitForDefault();
    VERIFY_IS_TRUE(pointerPressedEvent->HasFired());
}

void HitTestBasic::HitTestPlaneProjectionWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    HitTestPlaneProjectionCommon();
}

void HitTestBasic::HitTestPlaneProjectionCommon()
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ canvas;
    xaml_shapes::Rectangle^ rectangle = nullptr;

    RunOnUIThread([&]()
    {
        PlaneProjection^ projection = ref new PlaneProjection();
        projection->RotationY = 75;
        projection->CenterOfRotationX = 0;

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 300;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        rectangle->Projection = projection;

        canvas = ref new Canvas();
        canvas->Children->Append(rectangle);

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    {
        std::vector<wf::Point> points;
        points.push_back(wf::Point(40.0f, 50.0f)); // Center right - the projection shortens the width to about 50px
        points.push_back(wf::Point(40.0f, 115.0f)); // Bottom right - the projection makes the bottom right corner stick out below y=100

        // Touch hit testing uses a rect, so use 40px of leeway. 40+40 is still well within the bounds of the untransformed
        // rect (300 wide), and should catch a false hit if the projection isn't accounted for correctly.
        VerifyTappedPoints(canvas, rectangle, points, -40, 0, true, false);
    }
}

Canvas^ HitTestBasic::MakeCanvas(bool include3D, double left, double top, ::Windows::UI::Color color, Canvas^ child)
{
    // If we're including 3D, make sure it includes a non-zero transform. Otherwise hit testing could ignore 3D and still pass.
    double offsetIn3D = include3D ? 30 : 0;

    Canvas^ canvas = ref new Canvas();
    canvas->Width = 50;
    canvas->Height = 50;
    canvas->Background = ref new SolidColorBrush(color);
    Canvas::SetLeft(canvas, left - offsetIn3D);
    Canvas::SetTop(canvas, top - offsetIn3D);

    if (include3D)
    {
        CompositeTransform3D^ composite = ref new CompositeTransform3D();
        composite->TranslateX = offsetIn3D;
        composite->TranslateY = offsetIn3D;
        composite->TranslateZ = 0.1;
        canvas->Transform3D = composite;
    }

    if (child)
    {
        canvas->Children->Append(child);
    }

    return canvas;
}

void HitTestBasic::SetLTETransform(Microsoft::UI::Xaml::UIElement^ lte, bool include3D, double x, double y)
{
    if (include3D)
    {
        CompositeTransform3D^ composite = ref new CompositeTransform3D();
        composite->TranslateX = x;
        composite->TranslateY = y;
        composite->TranslateZ = 0.1;
        lte->Transform3D = composite;
    }
    else
    {
        TranslateTransform^ translate = ref new TranslateTransform();
        translate->X = x;
        translate->Y = y;
        lte->RenderTransform = translate;
    }
}

void HitTestBasic::HitTestPopupCommon(bool isParented, bool include3D, bool includeNestedPopup, bool specifySubtreeRootElement)
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ root;
    Canvas^ popupParent;
    Canvas^ popupChild;
    Canvas^ nestedPopupChild;
    xaml_shapes::Rectangle^ rectangle;
    Popup^ popup;
    Popup^ nestedPopup;

    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree");
        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        if (includeNestedPopup)
        {
            nestedPopupChild = ref new Canvas();
            nestedPopupChild->Children->Append(rectangle);

            nestedPopup = ref new Popup();
            nestedPopup->Child = nestedPopupChild;

            popupChild = ref new Canvas();
            popupChild->Children->Append(nestedPopup);
        }
        else
        {
            popupChild = ref new Canvas();
            popupChild->Children->Append(rectangle);
        }

        popup = ref new Popup();
        popup->Child = popupChild;

        popupParent = ref new Canvas();
        if (isParented)
        {
            Canvas::SetLeft(popupParent, 50);
            Canvas::SetTop(popupParent, 50);
            popupParent->Children->Append(popup);
        }
        else
        {
            popupParent->Width = 100;
            popupParent->Height = 100;
            popupParent->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

            // Since there's no popup parent to position, we have to set the offsets on the popup itself.
            popup->HorizontalOffset = 50;
            popup->VerticalOffset = 50;
        }

        root = ref new Canvas();
        if (include3D)
        {
            PerspectiveTransform3D^ perspective = ref new PerspectiveTransform3D();
            root->Transform3D = perspective;
        }
        root->Children->Append(popupParent);
        wh->WindowContent = root;

        if (!isParented)
        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                popup->XamlRoot = xamlRoot;
            }
        }
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popup");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    if (includeNestedPopup)
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Opening nested popup");
            nestedPopup->IsOpen = true;
        });
        wh->WaitForIdle();
    }

    wh->SynchronouslyTickUIThread(2);   // We rely on DComp property notifications to fill the hit testing stash. Wait for it.

    {
        LOG_OUTPUT(L"> Hit testing");

        std::vector<wf::Point> points;
        RunOnUIThread([&]()
        {
            points = GetHitTestingPoints(static_cast<FrameworkElement^>(rectangle), root);

        });
        wh->WaitForIdle();
        VerifyTappedPointsNoWindowSetup(specifySubtreeRootElement, rectangle, points);
    }

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Giving offset to Popup");
        // += instead of directly set because parentless popups will already have an offset of 50.
        popup->HorizontalOffset += 100;
        popup->VerticalOffset += 100;
        if (include3D)
        {
            CompositeTransform3D^ composite = ref new CompositeTransform3D();
            composite->TranslateZ = 0.1;
            popup->Transform3D = composite;
        }
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);

    {
        LOG_OUTPUT(L"> Hit testing");
        std::vector<wf::Point> points;
        
        RunOnUIThread([&]()
        {
            points = GetHitTestingPoints(static_cast<FrameworkElement^>(rectangle), root);
        });
        wh->WaitForIdle();

        VerifyTappedPointsNoWindowSetup(specifySubtreeRootElement, rectangle, points);
    }

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Giving RenderTransform to Popup.Child");
        TranslateTransform^ translate = ref new TranslateTransform();
        translate->X = -100;
        popupChild->RenderTransform = translate;
        if (include3D)
        {
            LOG_OUTPUT(L"  > Removing Transform3D on Popup.");
            popup->Transform3D = nullptr;

            CompositeTransform3D^ composite = ref new CompositeTransform3D();
            composite->TranslateZ = 0.1;
            popupChild->Transform3D = composite;
        }
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);

    {
        LOG_OUTPUT(L"> Hit testing");
        std::vector<wf::Point> points;
        RunOnUIThread([&]()
        {
            points = GetHitTestingPoints(static_cast<FrameworkElement^>(rectangle), root);
        });
        wh->WaitForIdle();
        VerifyTappedPointsNoWindowSetup(specifySubtreeRootElement, rectangle, points);
    }

    RunOnUIThread([&]()
    {
        if (isParented)
        {
            LOG_OUTPUT(L"> Giving RenderTransform to Popup parent");
            TranslateTransform^ translate = ref new TranslateTransform();
            translate->X = 100;
            translate->Y = -100;
            popupParent->RenderTransform = translate;
            if (include3D)
            {
                CompositeTransform3D^ composite = ref new CompositeTransform3D();
                composite->TranslateZ = 0.1;
                popupParent->Transform3D = composite;
            }
        }
        else
        {
            LOG_OUTPUT(L"> No Popup parent. Adjusting offset on parentless Popup to make the rest of the test work.");
            popup->HorizontalOffset += 100;
            popup->VerticalOffset -= 100;
        }
    });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);

    {
        LOG_OUTPUT(L"> Hit testing");
        std::vector<wf::Point> points;
        
        RunOnUIThread([&]()
        {
            points = GetHitTestingPoints(static_cast<FrameworkElement^>(rectangle), root);
        });
        wh->WaitForIdle();

        VerifyTappedPointsNoWindowSetup(specifySubtreeRootElement, rectangle, points);
    }

    if (includeNestedPopup)
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Giving offset to nested Popup");
            nestedPopup->VerticalOffset = 100;
            if (include3D)
            {
                CompositeTransform3D^ composite = ref new CompositeTransform3D();
                composite->TranslateZ = 0.1;
                nestedPopup->Transform3D = composite;
            }
        });
        wh->WaitForIdle();
        wh->SynchronouslyTickUIThread(2);

        {
            LOG_OUTPUT(L"> Hit testing");
            std::vector<wf::Point> points;
            RunOnUIThread([&]()
            {
                points = GetHitTestingPoints(static_cast<FrameworkElement^>(rectangle), root);
            });
            wh->WaitForIdle();
            VerifyTappedPointsNoWindowSetup(specifySubtreeRootElement, rectangle, points);
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Giving RenderTransform to nested Popup.Child");
            TranslateTransform^ translate = ref new TranslateTransform();
            translate->X = -100;
            nestedPopupChild->RenderTransform = translate;
            if (include3D)
            {
                CompositeTransform3D^ composite = ref new CompositeTransform3D();
                composite->TranslateZ = 0.1;
                nestedPopupChild->Transform3D = composite;
            }
        });
        wh->WaitForIdle();
        wh->SynchronouslyTickUIThread(2);

        {
            LOG_OUTPUT(L"> Hit testing");
            std::vector<wf::Point> points;
            RunOnUIThread([&]()
            {
                points = GetHitTestingPoints(static_cast<FrameworkElement^>(rectangle), root);
            });
            wh->WaitForIdle();
            VerifyTappedPointsNoWindowSetup(specifySubtreeRootElement, rectangle, points);
        }
    }
}

void HitTestBasic::HitTestParentedPopup_SubtreeRoot()
{
    HitTestPopupCommon(true /* isParented */, false /* include3D */, false /* includeNestedPopup */, true /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestParentedPopup3D_SubtreeRoot()
{
    HitTestPopupCommon(true /* isParented */, true /* include3D */, false /* includeNestedPopup */, true /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestParentedPopup_NullSubtreeRoot()
{
    HitTestPopupCommon(true /* isParented */, false /* include3D */, false /* includeNestedPopup */, false /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestParentedPopup3D_NullSubtreeRoot()
{
    HitTestPopupCommon(true /* isParented */, true /* include3D */, false /* includeNestedPopup */, false /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestNestedParentedPopup_SubtreeRoot()
{
    HitTestPopupCommon(true /* isParented */, false /* include3D */, true /* includeNestedPopup */, true /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestNestedParentedPopup3D_SubtreeRoot()
{
    HitTestPopupCommon(true /* isParented */, true /* include3D */, true /* includeNestedPopup */, true /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestNestedParentedPopup_NullSubtreeRoot()
{
    HitTestPopupCommon(true /* isParented */, false /* include3D */, true /* includeNestedPopup */, false /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestNestedParentedPopup3D_NullSubtreeRoot()
{
    HitTestPopupCommon(true /* isParented */, true /* include3D */, true /* includeNestedPopup */, false /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestParentlessPopup_SubtreeRoot()
{
    HitTestPopupCommon(false /* isParented */, false /* include3D */, false /* includeNestedPopup */, true /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestParentlessPopup3D_SubtreeRoot()
{
    HitTestPopupCommon(false /* isParented */, true /* include3D */, false /* includeNestedPopup */, true /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestParentlessPopup_NullSubtreeRoot()
{
    HitTestPopupCommon(false /* isParented */, false /* include3D */, false /* includeNestedPopup */, false /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestParentlessPopup3D_NullSubtreeRoot()
{
    HitTestPopupCommon(false /* isParented */, true /* include3D */, false /* includeNestedPopup */, false /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestNestedParentlessPopup_SubtreeRoot()
{
    HitTestPopupCommon(false /* isParented */, false /* include3D */, true /* includeNestedPopup */, true /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestNestedParentlessPopup3D_SubtreeRoot()
{
    HitTestPopupCommon(false /* isParented */, true /* include3D */, true /* includeNestedPopup */, true /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestNestedParentlessPopup_NullSubtreeRoot()
{
    HitTestPopupCommon(false /* isParented */, false /* include3D */, true /* includeNestedPopup */, false /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestNestedParentlessPopup3D_NullSubtreeRoot()
{
    HitTestPopupCommon(false /* isParented */, true /* include3D */, true /* includeNestedPopup */, false /* specifySubtreeRootElement */);
}

void HitTestBasic::HitTestPopupOpen_3DChild_PreTransformed()
{
    HitTestPopupOpen_3DChild(true);
}

void HitTestBasic::HitTestPopupOpen_3DChild_PostTransformed()
{
    HitTestPopupOpen_3DChild(false);
}

void HitTestBasic::HitTestPopupOpen_3DChild(bool preTransformed)
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ root;
    Canvas^ popupChild;
    xaml_shapes::Rectangle^ rectangle;
    Popup^ popup;

    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree");

        CompositeTransform3D^ composite = ref new CompositeTransform3D();
        composite->TranslateZ = 0.1;

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        popupChild = ref new Canvas();
        popupChild->Children->Append(rectangle);

        if (preTransformed)
        {
            popupChild->Transform3D = composite;
        }

        popup = ref new Popup();
        popup->Child = popupChild;

        if (!preTransformed)
        {
            popupChild->Transform3D = composite;
        }

        Canvas^ popupParent = ref new Canvas();
        Canvas::SetLeft(popupParent, 50);
        Canvas::SetTop(popupParent, 50);
        popupParent->Children->Append(popup);

        root = ref new Canvas();
        root->Children->Append(popupParent);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popup");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    wh->SynchronouslyTickUIThread(2);   // We rely on DComp property notifications to fill the hit testing stash. Wait for it.

    {
        LOG_OUTPUT(L"> Hit testing");
        std::vector<wf::Point> points;
        RunOnUIThread([&]()
        {
            points = GetHitTestingPoints(static_cast<FrameworkElement^>(rectangle), root);
        });
        wh->WaitForIdle();
        VerifyTappedPointsNoWindowSetup(true /* specifySubtreeRootElement */, rectangle, points);
    }
}

void HitTestBasic::HitTestLTECommon(bool include3D)
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ root;

    // An LTE that targets a direct sibling
    Canvas^ lte1Parent;
    Canvas^ lte1Target;
    UIElement^ lte1;

    // A nested LTE that targets a sibling's child
    Canvas^ lte2Parent;
    Canvas^ lte2TargetParent;
    Canvas^ lte2Target;
    UIElement^ lte2;

    // A nested LTE that targets a sibling. The target's parent itself is the target of lte2.
    Canvas^ lte3Target;
    UIElement^ lte3;

    // An LTE that sits
    Canvas^ popupLTETargetParent;
    Canvas^ popupLTETarget;
    UIElement^ popupLTE;
    Canvas^ absolutelyPositionedLTETarget;
    UIElement^ absolutelyPositionedLTE;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]()
        {
            if (popupLTE)
            {
                // Because popupLTE is rooted under the popup root, it won't be removed when we clean up the public root.
                // Remove it explicitly.
                LOG_OUTPUT(L"> Removing popupLTE");
                wh->RemoveTestLTE(popupLTE);
                popupLTE = nullptr;
            }
        });
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree");

        // This (300, 300) canvas offset will get overridden by the LTE
        lte3Target = MakeCanvas(include3D, 300, 300, Microsoft::UI::Colors::Purple, nullptr);

        // This (300, 300) canvas offset will get overridden by the LTE
        lte2Target = MakeCanvas(include3D, 300, 300, Microsoft::UI::Colors::Blue, lte3Target);
        lte2TargetParent = MakeCanvas(include3D, 100, 0, Microsoft::UI::Colors::Green, lte2Target);
        lte2Parent = MakeCanvas(include3D, 0, 100, Microsoft::UI::Colors::Yellow, lte2TargetParent);
        lte2 = wh->AddTestLTE(lte2Target, lte2Parent, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        SetLTETransform(lte2, include3D, 0, 100);

        // lte3's parent is itself targeted by lte2
        lte3 = wh->AddTestLTE(lte3Target, lte2Target, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        SetLTETransform(lte3, include3D, 100, 0);

        // This (300, 300) canvas offset will get overridden by the LTE
        lte1Target = MakeCanvas(include3D, 300, 300, Microsoft::UI::Colors::Orange, lte2Parent);
        lte1Parent = MakeCanvas(include3D, 50, 50, Microsoft::UI::Colors::Red, lte1Target);
        lte1 = wh->AddTestLTE(lte1Target, lte1Parent, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        SetLTETransform(lte1, include3D, 100, 0);

        // This (300, 300) canvas offset will get overridden by the LTE
        popupLTETarget = MakeCanvas(include3D, 300, 300, Microsoft::UI::Colors::White, nullptr);
        popupLTETargetParent = MakeCanvas(include3D, 50, 200, Microsoft::UI::Colors::Gray, popupLTETarget);

        root = ref new Canvas();
        if (include3D)
        {
            root->Transform3D = ref new PerspectiveTransform3D();
        }
        root->Children->Append(lte1Parent);
        root->Children->Append(popupLTETargetParent);
        wh->WindowContent = root;

        // Give the popup root a child collection, so that it can create a transition root off of that.
        auto popup = ref new Popup();
        popup->Child = ref new xaml_shapes::Rectangle();
        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                popup->XamlRoot = xamlRoot;
            }
        }
        popup->IsOpen = true;
        popup->IsOpen = false;

        popupLTE = wh->AddTestLTE(popupLTETarget, nullptr, LTEParentMode::PopupRoot, false /* isAbsolutelyPositioned */);
        SetLTETransform(popupLTE, include3D, 0, 150);

        // This (0, 100) canvas offset will get overridden by the LTE
        absolutelyPositionedLTETarget = MakeCanvas(include3D, 0, 100, Microsoft::UI::Colors::Brown, nullptr);
        lte2TargetParent->Children->Append(absolutelyPositionedLTETarget);
        absolutelyPositionedLTE = wh->AddTestLTE(absolutelyPositionedLTETarget, lte2TargetParent, LTEParentMode::NormalTree, true /* isAbsolutelyPositioned */);
        SetLTETransform(absolutelyPositionedLTE, include3D, 250, 0);
    });
    wh->WaitForIdle();

    {
        LOG_OUTPUT(L"> Hit testing lte1Target");
        std::vector<wf::Point> points;
        points.push_back(wf::Point(153.0f, 53.0f));
        points.push_back(wf::Point(197.0f, 53.0f));
        points.push_back(wf::Point(153.0f, 97.0f));
        VerifyTappedPointsNoWindowSetup(true /* specifySubtreeRootElement */, lte1Target, points);
    }

    {
        LOG_OUTPUT(L"> Hit testing nested lte2Target");
        std::vector<wf::Point> points;
        points.push_back(wf::Point(253.0f, 253.0f));
        points.push_back(wf::Point(297.0f, 253.0f));
        points.push_back(wf::Point(253.0f, 297.0f));
        VerifyTappedPointsNoWindowSetup(true /* specifySubtreeRootElement */, lte2Target, points);
    }

    {
        LOG_OUTPUT(L"> Hit testing nested lte3Target");
        std::vector<wf::Point> points;
        points.push_back(wf::Point(353.0f, 253.0f));
        points.push_back(wf::Point(397.0f, 253.0f));
        points.push_back(wf::Point(353.0f, 297.0f));
        VerifyTappedPointsNoWindowSetup(true /* specifySubtreeRootElement */, lte3Target, points);
    }

    {
        LOG_OUTPUT(L"> Hit testing popupLTETarget");
        std::vector<wf::Point> points;
        points.push_back(wf::Point(53.0f, 353.0f));
        points.push_back(wf::Point(97.0f, 353.0f));
        points.push_back(wf::Point(53.0f, 397.0f));
        VerifyTappedPointsNoWindowSetup(true /* specifySubtreeRootElement */, popupLTETarget, points);
    }

    {
        LOG_OUTPUT(L"> Skip hit testing absolutelyPositionedLTETarget");
        // Absolutely positioned LTEs don't copy transforms from the tree via SetTransformParent2. They only write their
        // own local transforms. This LTE is parented under the orange lte1, which has offset (150, 50). The LTE adds
        // its absolute transform of (250, 0), which puts it at (400, 50).
        // ^ This behavior seems flaky. It depends on whichever ancestor happens to have a comp node. In the 3D version
        //      of this test, the green element has a comp node because of a Transform3D, and the absolutely positioned
        //      LTE renders in a different place.
        //
        //   Revisit absolutely positioned LTEs. Maybe they should be doing SetTransformParent2 to the root comp node.
        //
        //   They also really mess up bounds checking in the hit testing walk. All LTEs mess up bounds checking, but
        //      especially absolutely positioned LTEs. Child bounds don't include LTEs, and if they did, then the LTE's
        //      outer bounds needs to be aware of where the LTE is attached. In this test, lte2's outer bounds are tricky.
        std::vector<wf::Point> points;
        points.push_back(wf::Point(403.0f, 53.0f));
        points.push_back(wf::Point(447.0f, 53.0f));
        points.push_back(wf::Point(403.0f, 97.0f));
//        VerifyTappedPointsNoWindowSetup(root, absolutelyPositionedLTETarget, points);
    }
}


void HitTestBasic::HitTestLTE()
{
    HitTestLTECommon(false /* include3D */);
}

void HitTestBasic::HitTestLTE3D()
{
    HitTestLTECommon(true /* include3D */);
}

void HitTestBasic::NewLTETargetingExisting3D()
{
    // Regression test
    // An LTE that targets an existing 3D branch of the tree isn't labeling itself as 3D. We weren't
    // picking up its transform as part of building a world transform down to a 3D element, so we
    // didn't correctly transform the world-space point down to that 3D subtree.

    const auto& wh = TestServices::WindowHelper;

    Canvas^ root;
    Canvas^ has3D;
    Canvas^ parent;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");

        has3D = MakeCanvas(true, 100, 100, Microsoft::UI::Colors::Red, nullptr);

        parent = ref new Canvas();
        parent->Children->Append(has3D);

        root = ref new Canvas();
        root->Children->Append(parent);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    {
        LOG_OUTPUT(L"> Hit testing tree.");
        std::vector<wf::Point> points;
        points.push_back(wf::Point(103.0f, 103.0f));
        points.push_back(wf::Point(147.0f, 103.0f));
        points.push_back(wf::Point(103.0f, 147.0f));
        VerifyTappedPointsNoWindowSetup(false /* specifySubtreeRootElement */, has3D, points);
    }

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding LTE targeting existing 3D branch.");
        wh->AddTestLTE(parent, root, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();

    {
        LOG_OUTPUT(L"> Hit testing tree with LTE.");
        std::vector<wf::Point> points;
        points.push_back(wf::Point(103.0f, 103.0f));
        points.push_back(wf::Point(147.0f, 103.0f));
        points.push_back(wf::Point(103.0f, 147.0f));
        VerifyTappedPointsNoWindowSetup(false /* specifySubtreeRootElement */, has3D, points);
    }
}

void HitTestBasic::LTESkipsSubtreeCommon(bool isDepthOnLTE)
{
    // Here's the scenario we're trying to catch:
    //
    //  <Root>
    //      <A>
    //          <B Offset="100">
    //              <C>
    //                  <D Transform3D="...">
    //                      ...
    //                  </D>
    //              </C>
    //          </B>
    //          <LTE Target="C" />
    //      </A>
    //  </Root>
    //
    // Element D has a 3D transform and needs the transforms of all its ancestors to be collected. It's rendering inside
    // a redirected tree (LTE, which points to its parent C). The transforms that we're after are Root-A-B-LTE-D. The
    // tricky element here is B, because if we just follow the ancestor chain and LTEs up from D, we'll reach C, then LTE,
    // then skip directly to A (LTE's parent). We need to mark B with 3D depth as well, because we need its offset when
    // doing redirected hit testing.
    //
    // An alternative scenario puts the Transform3D on the LTE instead of on D.

    const auto& wh = TestServices::WindowHelper;

    Canvas^ d;
    UIElement^ lte;
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating tree.");

                d = MakeCanvas(false /* include3D */, 100, 100, Microsoft::UI::Colors::Blue, nullptr);
        Canvas^ c = MakeCanvas(false /* include3D */, 0, 0, Microsoft::UI::Colors::Green, d);
        Canvas^ b = MakeCanvas(false /* include3D */, 200, 100, Microsoft::UI::Colors::Yellow, c);
        Canvas^ a = MakeCanvas(false /* include3D */, 0, 0, Microsoft::UI::Colors::Red, b);

        lte = wh->AddTestLTE(c, a, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);

        root = ref new Canvas();
        root->Children->Append(a);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Adding Transform3D.");

        CompositeTransform3D^ transform = ref new CompositeTransform3D();
        transform->TranslateZ = 20;
        if (isDepthOnLTE)
        {
            lte->Transform3D = transform;
        }
        else
        {
            d->Transform3D = transform;
        }
    });
    wh->WaitForIdle();

    {
        LOG_OUTPUT(L"> Hit testing");
        std::vector<wf::Point> points;
        points.push_back(wf::Point(303.0f, 203.0f));
        points.push_back(wf::Point(347.0f, 203.0f));
        points.push_back(wf::Point(303.0f, 247.0f));
        VerifyTappedPointsNoWindowSetup(true /* specifySubtreeRootElement */, d, points);
    }
}

void HitTestBasic::LTESkipsSubtree_DepthUnderLTE()
{
    LTESkipsSubtreeCommon(false /* isDepthOnLTE */);
}

void HitTestBasic::LTESkipsSubtree_DepthOnLTE()
{
    LTESkipsSubtreeCommon(true /* isDepthOnLTE */);
}

void HitTestBasic::DManipHitTestVisual()
{
    WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ scrollViewerRoot;
    Canvas^ compNode;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating tree. It draws nothing, so there should not be a DManip hit test visual.");

        scrollViewerRoot = ref new Canvas();
        scrollViewerRoot->Width = 300;
        scrollViewerRoot->Height = 300;

        // Use an explicit ScrollViewer for this test. The RootScrollViewer isn't guaranteed to have a DManip hit testing
        // visual until a manipulation actually begins.
        ScrollViewer^ scrollViewer = ref new ScrollViewer();
        scrollViewer->Width = 100;
        scrollViewer->Height = 100;
        scrollViewer->Background = nullptr; // Remove default background
        scrollViewer->Content = scrollViewerRoot;

        // Use a Canvas for the actual root of the tree to avoid the RootScrollViewer and to simplify the MockDComp output.
        Canvas^ root = ref new Canvas();
        root->Children->Append(scrollViewer);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"EmptyTree");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Add an empty child to the tree. It still draws nothing, so there should not be a DManip hit test visual.");

        Canvas^ empty = ref new Canvas();
        empty->Width = 50;
        empty->Height = 50;
        scrollViewerRoot->Children->Append(empty);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"EmptyTree");    // The new canvas should not affect output

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Add an empty comp node to the tree. It still draws nothing, so there should not be a DManip hit test visual.");

        compNode = ref new Canvas();
        compNode->Width = 50;
        compNode->Height = 50;
        compNode->CompositeMode = ElementCompositeMode::SourceOver;
        scrollViewerRoot->Children->Append(compNode);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"EmptyTree2");   // Got a new comp node

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Add content to the tree. There should be a DManip hit test visual now.");

        Canvas^ content = MakeCanvas(false /* include3D */, 0, 0, Microsoft::UI::Colors::Green, nullptr);
        compNode->Children->Append(content);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Content");
}

void HitTestBasic::ProgrammaticHitTestWithEmptyTree()
{
    const auto& wh = TestServices::WindowHelper;

    // Recover from resetting the visual tree at the end
    auto shutdownGuard = wil::scope_exit([wh]
    {
        wh->ShutdownXaml();
        wh->InitializeXaml();
    });

    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();
        root->Width = 100;
        root->Height = 100;
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Reset the visual tree. Don't crash.");
    wh->ResetVisualTree();

    RunOnUIThread([&]()
    {
        wf::Rect rect(0, 0, 1, 1);
        auto canvas = ref new Canvas();

        HRESULT hr = S_OK;
        LOG_OUTPUT(L"> Calling rect FindElementsInHostCoordinates with a null RootVisual. Don't crash.");
        try
        {
            auto rectHitTest = VisualTreeHelper::FindElementsInHostCoordinates(rect, canvas);
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }

        LOG_OUTPUT(L"> Expecting E_FAIL when calling FindElementsInHostCoordinates with a null RootVisual.");
        VERIFY_ARE_EQUAL(hr, E_FAIL);
    });
}

// return points of bounds of given UIElement in the coordinate space of the XamlRoot of root
// need to run only in UI thread
std::vector<wf::Point> HitTestBasic::GetHitTestingPoints(FrameworkElement^ element, UIElement^ root, float offset)
{
    std::vector<wf::Point> points;
    auto transform = element->TransformToVisual(root);
    points.push_back(transform->TransformPoint(wf::Point(0 + offset, 0 + offset)));
    points.push_back(transform->TransformPoint(wf::Point(0 + static_cast<float>(element->Width) - offset, 0 + offset)));
    points.push_back(transform->TransformPoint(wf::Point(0 + offset, 0 + static_cast<float>(element->Height) - offset)));
    LOG_OUTPUT(L"  > Hit testing points: topleft : %lf,%lf topright : %lf,%lf bottomleft: %lf,%lf",\
                            points[0].X, points[0].Y, points[1].X, points[1].Y, points[2].X, points[2].Y);
    return points;
}

} } } } } } }
