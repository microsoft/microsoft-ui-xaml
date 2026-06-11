// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "PointerPositionPropertySetTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include <SafeEventRegistration.h>
#include <TestEvent.h>

using namespace test_infra; // TestServices
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics;

namespace wfn = ::Windows::Foundation::Numerics;

bool PointerPositionPropertySetTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool PointerPositionPropertySetTests::ClassCleanup()
{
    return true;
}

bool PointerPositionPropertySetTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool PointerPositionPropertySetTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void PointerPositionPropertySetTests::Basic()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Grid ^rootElement;
    xaml_shapes::Rectangle^ rect;
    auto canvasPointerPressedRegistration = CreateSafeEventRegistration(UIElement, PointerPressed);
    auto targetPointerPressedRegistration = CreateSafeEventRegistration(UIElement, PointerPressed);
    auto pointerPressedEvent = std::make_shared<Event>();
    bool targetPointerPressed = false;

    ExpressionAnimation ^animation;
    CompositionPropertySet ^pointerPositionPS;

    RunOnUIThread([&]()
    {
        rootElement = ref new xaml_controls::Grid();
        rootElement->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        rootElement->Clip = ref new xaml_media::RectangleGeometry();
        rootElement->Clip->Rect = {0,0,300,300};

        TestServices::WindowHelper->WindowContent = rootElement;

        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        rect->VerticalAlignment = VerticalAlignment::Top;
        rect->HorizontalAlignment = HorizontalAlignment::Left;
        rect->Width = 100;
        rect->Height = 100;
        rootElement->Children->Append(rect);

        pointerPositionPS = ElementCompositionPreview::GetPointerPositionPropertySet(rootElement);
        VERIFY_IS_NOT_NULL(pointerPositionPS);

        Visual ^visual = ElementCompositionPreview::GetElementVisual(rect);
        animation = visual->Compositor->CreateExpressionAnimation();
        animation->Expression = L"Matrix4x4.CreateFromTranslation(vector3(pointer.Position.x - 50, pointer.Position.y - 50, 0))";
        animation->SetReferenceParameter("pointer", pointerPositionPS);
        visual->StartAnimation("TransformMatrix", animation);
    });

    targetPointerPressedRegistration.Attach(
        rect,
        ref new xaml_input::PointerEventHandler(
        [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
    {
        LOG_OUTPUT(L"Target PointerPressed event received");
        args->Handled = true;
        targetPointerPressed = true;
        pointerPressedEvent->Set();
    }));

    canvasPointerPressedRegistration.Attach(
        rootElement,
        ref new xaml_input::PointerEventHandler(
        [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
    {
        LOG_OUTPUT(L"Background PointerPressed event received");
        args->Handled = true;
        pointerPressedEvent->Set();
    }));

    for (int i = 0; i < 42; ++i)
    {
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(wf::Point(150, 150));
        pointerPressedEvent->WaitForDefault();

        if (targetPointerPressed)
        {
            break;
        }
    }

    VERIFY_IS_TRUE(targetPointerPressed);
}

void PointerPositionPropertySetTests::TouchUpdate()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& ih = TestServices::InputHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ root;
    xaml_shapes::Rectangle^ rect;
    xaml_shapes::Rectangle^ rect2;

    ExpressionAnimation ^animation;
    CompositionPropertySet ^pointerPositionPS;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();

        wh->WindowContent = root;

        rect2 = ref new xaml_shapes::Rectangle();
        rect2->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        rect2->Width = 10;
        rect2->Height = 10;
        root->Children->Append(rect2);

        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        rect->Width = 200;
        rect->Height = 200;
        root->Children->Append(rect);

        pointerPositionPS = ElementCompositionPreview::GetPointerPositionPropertySet(rect);
        VERIFY_IS_NOT_NULL(pointerPositionPS);

        // We don't actually expose a value in pointerPositionPS. Rather, it has an ExpressionAnimation that's animating
        // "Position", and that ExpressionAnimation is backed by a PropertySet named "hover". That "hover" property set
        // is updated by Xaml. Since we can't verify it programmatically, we animate a rectangle with it and check the
        // MockDComp output instead.
        Visual ^visual = ElementCompositionPreview::GetElementVisual(rect2);
        animation = visual->Compositor->CreateExpressionAnimation();
        animation->Expression = L"Matrix4x4.CreateFromTranslation(vector3(pointer.Position.x, pointer.Position.y, 0))";
        animation->SetReferenceParameter("pointer", pointerPositionPS);
        visual->StartAnimation("TransformMatrix", animation);
    });
    wh->WaitForIdle();

    ih->Tap(rect);
    wh->SynchronouslyTickUIThread(10);

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"tap");

    ih->PressHoldAndPanFromCenter(rect, 75 /*relX*/, 75 /*relY*/, 1.0 /*velocityFactor*/, 500 /*holdTime*/);
    wh->SynchronouslyTickUIThread(10);

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"drag");
}
