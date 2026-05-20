// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TouchTargeting.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "collection.h"
#include <ChangeDPI.h>

using namespace std;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace ::Windows::Foundation::Collections;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

Platform::String^ TouchTargeting::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\HitTest\\";
}

bool TouchTargeting::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool TouchTargeting::TestCleanup()
{
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

// Tap just to the right of a single Rectangle
void TouchTargeting::Rectangle()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"Rectangle.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    xaml_shapes::Rectangle^ rectangle = nullptr;
    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    auto pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"myRectangle"));
        VERIFY_IS_NOT_NULL(rectangle);

        pointerPressedRegistration.Attach(
            rectangle,
            ref new xaml_input::PointerEventHandler(
            [pointerPressedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"PointerPressed raised.");
            pointerPressedEvent->Set();
        }));
    });

    // 1.05 yields a small delta to the right edge of the Rectangle
    // We expect touch targeting to adjust to inside the Rectangle
    TestServices::InputHelper->Tap(rectangle, 1.05f, 0.5f);
    pointerPressedEvent->WaitForDefault();
}

// Tap between two Rectangles, favoring the Rectangle on the left
void TouchTargeting::TwoRectangles()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TwoRectangles.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    xaml_shapes::Rectangle^ rectangle = nullptr;
    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    auto pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"rectangle1"));
        VERIFY_IS_NOT_NULL(rectangle);

        pointerPressedRegistration.Attach(
            rectangle,
            ref new xaml_input::PointerEventHandler(
            [pointerPressedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"PointerPressed raised.");
            pointerPressedEvent->Set();
        }));
    });

    // 1.05 yields a point just to the right of the left-hand Rectangle
    // We expect touch targeting to adjust the point to inside the left-hand Rectangle
    TestServices::InputHelper->Tap(rectangle, 1.05f, 0.5f);
    pointerPressedEvent->WaitForDefault();
}

// Tap a Rectangle inside a Canvas with a background (2 cases)
void TouchTargeting::CanvasAndRectangle1()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CanvasAndRectangle1.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    // Case 1: Tap just to the right of the Rectangle
    {
        xaml_shapes::Rectangle^ rectangle = nullptr;
        auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
        auto pointerPressedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"rectangle"));
            VERIFY_IS_NOT_NULL(rectangle);

            pointerPressedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [pointerPressedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PointerPressed raised.");
                pointerPressedEvent->Set();
            }));
        });

        // 1.05 yields a point just to the right of the Rectangle, but also inside the Canvas
        // We expect touch targeting to adjust to inside the Rectangle
        TestServices::InputHelper->Tap(rectangle, 1.05f, 0.5f);
        pointerPressedEvent->WaitForDefault();
    }

    // Case 2:  Tap just to the right of the Canvas
    {
        Canvas^ innerCanvas = nullptr;
        xaml_shapes::Rectangle^ rectangle = nullptr;
        auto pointerPressedRegistrationCanvas = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
        auto pointerPressedRegistrationRectangle = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
        auto pointerPressedEvent = std::make_shared<Event>();
        bool pointerPressedOnRectangle = false;

        RunOnUIThread([&]()
        {
            innerCanvas = safe_cast<Canvas^>(canvas->FindName(L"innerCanvas"));
            VERIFY_IS_NOT_NULL(innerCanvas);

            rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"rectangle"));
            VERIFY_IS_NOT_NULL(rectangle);

            pointerPressedRegistrationCanvas.Attach(
                innerCanvas,
                ref new xaml_input::PointerEventHandler(
                [pointerPressedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PointerPressed raised on Canvas.");
                pointerPressedEvent->Set();
            }));

            pointerPressedRegistrationRectangle.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PointerPressed raised on Rectangle.  This is unexpected!");
                pointerPressedOnRectangle = true;
            }));
        });

        // 1.05 yields a point just to the right of the Canvas.
        // We expect touch targeting to adjust to inside the Canvas, and not see the Rectangle
        TestServices::InputHelper->Tap(innerCanvas, 1.05f, 0.5f);
        pointerPressedEvent->WaitForDefault();
        VERIFY_IS_FALSE(pointerPressedOnRectangle);
    }
}

// Tap just to the right of a Canvas with Background and child Rectangle
void TouchTargeting::CanvasAndRectangle2()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CanvasAndRectangle2.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    Canvas^ innerCanvas = nullptr;
    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    auto pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        innerCanvas = safe_cast<Canvas^>(canvas->FindName(L"innerCanvas"));
        VERIFY_IS_NOT_NULL(innerCanvas);

        xaml_shapes::Rectangle^ rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"rectangle"));
        VERIFY_IS_NOT_NULL(rectangle);

        pointerPressedRegistration.Attach(
            rectangle,
            ref new xaml_input::PointerEventHandler(
            [pointerPressedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"PointerPressed raised.");
            pointerPressedEvent->Set();
        }));
    });

    // 1.025 yields a point that is just to the right of the Canvas, but the touch geometry
    // is large enough to hit the inner Rectangle.
    // We expect touch targeting to adjust to inside the Rectangle
    TestServices::InputHelper->Tap(innerCanvas, 1.025f, 0.5f);
    pointerPressedEvent->WaitForDefault();
}

// Tap inside two overlapping Rectangles (2 cases)
void TouchTargeting::OverlappingRectangles()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"OverlappingRectangles.xaml"));
    VERIFY_IS_NOT_NULL(canvas);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = canvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    xaml_shapes::Rectangle^ rectangle = nullptr;

    // Case 1:  Tap in the overlapping area, but inside the right-hand Rectangle
    {
        auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
        auto pointerPressedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"rectangle2"));
            VERIFY_IS_NOT_NULL(rectangle);

            pointerPressedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [pointerPressedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PointerPressed raised.");
                pointerPressedEvent->Set();
            }));
        });

        // 0.05 yields a point just inside the left edge of the right-hand rectangle.
        // We expect touch targeting to do no adjusting at all
        TestServices::InputHelper->Tap(rectangle, 0.05f, 0.5f);
        pointerPressedEvent->WaitForDefault();
    }

    // Case 2:  Tap inside the left-hand Rectangle near the place where the two Rectangles overlap
    {
        auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
        auto pointerPressedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            rectangle = safe_cast<xaml_shapes::Rectangle^>(canvas->FindName(L"rectangle1"));
            VERIFY_IS_NOT_NULL(rectangle);

            pointerPressedRegistration.Attach(
                rectangle,
                ref new xaml_input::PointerEventHandler(
                [pointerPressedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
            {
                LOG_OUTPUT(L"PointerPressed raised.");
                pointerPressedEvent->Set();
            }));
        });

        // 0.45 yields a point that's inside the left-hand Rectangle, and also close enough that
        // the contact geometry intersects the right-hand Rectangle.
        // In this case we expect touch targeting to ignore the right-hand Rectangle and
        // not perform any adjustment.
        TestServices::InputHelper->Tap(rectangle, 0.45f, 0.5f);
        pointerPressedEvent->WaitForDefault();
    }
}

void TouchTargeting::SimulateOneCoreTransforms_Plateau()
{
    // Note: OneCore and WindowsCore VMs have OneCoreTransforms enabled, but they don't raise the OnTouchHitTesting event
    // required for fuzzy hit testing. The only VM that raises that event is desktop, so we simulate the OneCoreTransform
    // setup on desktop.

    ChangeDPI changeDPI;

    const auto& wh = TestServices::WindowHelper;
    Xaml::XamlRoot^ xamlRoot;
    float plateauScale = 0.0f;

    RunOnUIThread([&]() {
        auto roots = test_infra::TestServices::WindowHelper->GetXamlRoots();
        xamlRoot = roots->GetAt(0);
    });

    plateauScale = (float)xamlRoot->RasterizationScale;
    LOG_OUTPUT(L"System DPI is %f", plateauScale);
    VERIFY_ARE_NOT_EQUAL(plateauScale, 1.0f);

    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    ::Windows::Foundation::Size size(400, 400);
    wh->SetWindowSizeOverride(size);

    xaml_shapes::Rectangle^ rectangle;
    xaml_shapes::Rectangle^ injectTapRectangle;

    RunOnUIThread([&]()
    {
        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        Canvas::SetLeft(rectangle, 100);
        Canvas::SetTop(rectangle, 100);

        //
        // This element exists to work around a test problem.
        //
        // The OnTouchHitTesting event used for fuzzy hit testing provides the touch region with the plateau scale
        // already divided out. Here, we're setting the system plateau to 1.25 scale,
        // so the fuzzy hit testing handler will just read whatever point we inject. The fuzzy hit testing
        // handler then multiplies the point by the scale that we set on the root visual, which is always 1x in
        // OneCoreTransforms mode.
        //
        // The injection is done with WindowHelper::ConvertToPhysicalPixels, which reads the system zoom scale
        // from Xaml. If we want to inject a tap at 100px for an element and the system zoom scale is 1.25, it
        // will be injected as 125. This means we miss the element, which is still at 100 because the scale on the
        // root is 1x.
        //
        // To work around this, we inject a tap at 50px, which gets scaled up by ConvertToPhysicalPixels to 62px.
        // We use a different element to inject this tap to make it more robust than calculating a raw point. The
        // "injectTapRectangle" is this element.
        //
        // This is also why we can't test fuzzy hit testing outside OneCoreTransforms mode. In the same example, we
        // have the zoom scale at 1.25x, so there's a 1.25x scale on the root visual in non-OCT mode. The element is on
        // screen at 125px. Fuzzy hit testing expects an input at 100px, which it multiplies by the root visual scale
        // (1.25x in non-PCT) to get 125, which it then runs down the tree to do the hit testing. Meanwhile, the actual
        // hit testing expects an input at 125, which it runs down the tree directly. This produces conflicting
        // requirements: fuzzy hit testing expects 100, but actual hit testing expects 125. The only way to produce
        // this is to set the actual plateau scale to 125%
        //
        injectTapRectangle = ref new xaml_shapes::Rectangle();
        injectTapRectangle->Width = 100 / plateauScale;
        injectTapRectangle->Height = 100 / plateauScale;
        // No Fill property - this element doesn't hit test.
        Canvas::SetLeft(injectTapRectangle, 100 / plateauScale);
        Canvas::SetTop(injectTapRectangle, 100 / plateauScale);

        Canvas^ root = ref new Canvas();
        root->Children->Append(rectangle);
        root->Children->Append(injectTapRectangle);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    auto pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        pointerPressedRegistration.Attach(
            rectangle,
            ref new xaml_input::PointerEventHandler(
            [pointerPressedEvent](Platform::Object^, xaml_input::PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"PointerPressed raised.");
            pointerPressedEvent->Set();
        }));
    });

    // 1.05 yields a small delta to the right edge of the Rectangle
    // We expect touch targeting to adjust to inside the Rectangle
    TestServices::InputHelper->Tap(injectTapRectangle, 1.05f, 0.5f);
    pointerPressedEvent->WaitForDefault();
}

} } } } } } }
