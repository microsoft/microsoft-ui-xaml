// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CanvasIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Canvas {

    bool CanvasIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CanvasIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CanvasIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void CanvasIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::Canvas>::CanInstantiate();
    }

    void CanvasIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::Canvas>::CanEnterAndLeaveLiveTree();
    }

    void CanvasIntegrationTests::CanPlaceElements()
    {
        TestCleanupWrapper cleanup;

        Platform::Collections::Vector<xaml::UIElement^>^ elementsVector = nullptr;
        xaml_shapes::Rectangle^ rect1 = nullptr;
        xaml_shapes::Rectangle^ rect2 = nullptr;
        wf::Point rect1ExpectedTopLeftCorner(100, 25);
        wf::Point rect2ExpectedTopLeftCorner(15, 150);

        RunOnUIThread([&]()
        {
            rect1 = CreateRectangle();
            rect2 = CreateRectangle();

            xaml_controls::Canvas::SetLeft(rect1, rect1ExpectedTopLeftCorner.X);
            xaml_controls::Canvas::SetTop(rect1, rect1ExpectedTopLeftCorner.Y);

            xaml_controls::Canvas::SetLeft(rect2, rect2ExpectedTopLeftCorner.X);
            xaml_controls::Canvas::SetTop(rect2, rect2ExpectedTopLeftCorner.Y);


            elementsVector = ref new Platform::Collections::Vector<xaml::UIElement^>();

            elementsVector->Append(rect1);
            elementsVector->Append(rect2);
        });

        auto canvas = AddCanvasWithElements(elementsVector);

        RunOnUIThread([&]()
        {
            wf::Point rect1ActualTopLeftCorner;
            wf::Point rect2ActualTopLeftCorner;

            auto rect1Transform = rect1->TransformToVisual(canvas);
            auto rect2Transform = rect2->TransformToVisual(canvas);

            rect1ActualTopLeftCorner = rect1Transform->TransformPoint(wf::Point(0, 0));
            rect2ActualTopLeftCorner = rect2Transform->TransformPoint(wf::Point(0, 0));

            LOG_OUTPUT(L"Rect 1: Actual corner = (%.2f, %.2f); Expected corner = (%.2f, %.2f).", rect1ActualTopLeftCorner.X, rect1ActualTopLeftCorner.Y, rect1ExpectedTopLeftCorner.X, rect1ExpectedTopLeftCorner.Y);
            LOG_OUTPUT(L"Rect 2: Actual corner = (%.2f, %.2f); Expected corner = (%.2f, %.2f).", rect2ActualTopLeftCorner.X, rect2ActualTopLeftCorner.Y, rect2ExpectedTopLeftCorner.X, rect2ExpectedTopLeftCorner.Y);

            VERIFY_ARE_EQUAL(rect1ExpectedTopLeftCorner, rect1ActualTopLeftCorner);
            VERIFY_ARE_EQUAL(rect2ExpectedTopLeftCorner, rect2ActualTopLeftCorner);
        });
    }

    xaml_controls::Canvas^ CanvasIntegrationTests::AddCanvasWithElements(Platform::Collections::Vector<xaml::UIElement^>^ elementsVector)
    {
        xaml_controls::Canvas^ canvas = nullptr;
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, Loaded);
        auto loadedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Adding the Canvas to the visual tree with %d elements.", elementsVector->Size);
            canvas = ref new xaml_controls::Canvas();

            canvas->Width = 200;
            canvas->Height = 200;

            for (auto element : elementsVector)
            {
                canvas->Children->Append(element);
            }

            loadedRegistration.Attach(canvas, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = canvas;
        });

        LOG_OUTPUT(L"Waiting for the Canvas to be loaded...");
        loadedEvent->WaitForDefault();
        LOG_OUTPUT(L"Canvas loaded.");

        return canvas;
    }

    xaml_shapes::Rectangle^ CanvasIntegrationTests::CreateRectangle()
    {
        auto rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        rectangle->Width = 100;
        rectangle->Height = 100;

        return rectangle;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Button
