// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "XamlWinRTCompInteropManipTransformTests.h"
#include "TestCleanupWrapper.h"

using namespace test_infra;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Composition;

namespace local = Microsoft::UI::Xaml::Tests::Foundation::Graphics;

bool local::XamlWinRTCompInteropManipTransformTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool local::XamlWinRTCompInteropManipTransformTests::ClassCleanup()
{
    return true;
}

bool local::XamlWinRTCompInteropManipTransformTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool local::XamlWinRTCompInteropManipTransformTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void local::XamlWinRTCompInteropManipTransformTests::BasicGetScrollViewerManipulationPropertySet()
{
    TestCleanupWrapper cleanup;

    ScrollViewer ^scrollViewer;
    CompositionPropertySet ^ps1;
    CompositionPropertySet ^ps2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Create ScrollViewer");
        scrollViewer = ref new ScrollViewer();

        LOG_OUTPUT(L"Query manipulation PropertySet");
        ps1 = ElementCompositionPreview::GetScrollViewerManipulationPropertySet(scrollViewer);
        VERIFY_IS_NOT_NULL(ps1, L"Should be able to obtain manipulation transform before joining visual tree");

        LOG_OUTPUT(L"Join the visual tree");
        TestServices::WindowHelper->WindowContent = scrollViewer;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        ps2 = ElementCompositionPreview::GetScrollViewerManipulationPropertySet(scrollViewer);
        VERIFY_IS_NOT_NULL(ps2, L"Should be able to obtain manipulation transform after joining the visual tree");
        VERIFY_IS_TRUE(ps1 == ps2, L"Once created the instance is never expected to change");
    });
}

void local::XamlWinRTCompInteropManipTransformTests::AnimateVisualOffset()
{
    TestCleanupWrapper cleanup;

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    Grid ^rootPanel;
    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<Grid^>(xaml_markup::XamlReader::Load(LR"(
            <Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                  xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
              <ScrollViewer x:Name="ScrollViewer1">
                <StackPanel>
                  <Border Height="300" Width="300" Background="Red"/>
                  <Border Height="300" Width="300" Background="Green"/>
                  <Border Height="300" Width="300" Background="Blue"/>
                </StackPanel>
              </ScrollViewer>
              <Border x:Name="Border1"
                      HorizontalAlignment="Left"
                      Height="40"
                      Width="40"
                      Background="Yellow"/>
            </Grid>
        )"));
        VERIFY_IS_NOT_NULL(rootPanel);
    });

    ScrollViewer ^scrollViewer1;
    Border ^border1;

    auto loadedRegistration = CreateSafeEventRegistration(Grid, Loaded);
    auto loadedEvent = std::make_shared<Event>();

    CompositionPropertySet ^manipulation;
    ExpressionAnimation ^animation;

    loadedRegistration.Attach(
        rootPanel,
        ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
    {
        LOG_OUTPUT(L"Loaded event received");

        scrollViewer1 = safe_cast<ScrollViewer^>(rootPanel->FindName(L"ScrollViewer1"));
        VERIFY_IS_NOT_NULL(scrollViewer1);

        border1 = safe_cast<Border^>(rootPanel->FindName(L"Border1"));
        VERIFY_IS_NOT_NULL(border1);

        manipulation = ElementCompositionPreview::GetScrollViewerManipulationPropertySet(scrollViewer1);
        VERIFY_IS_NOT_NULL(manipulation);
        Visual ^visual = ElementCompositionPreview::GetElementVisual(border1);
        VERIFY_IS_NOT_NULL(visual);
        animation = visual->Compositor->CreateExpressionAnimation();
        VERIFY_IS_NOT_NULL(animation);
        animation->Expression = L"Matrix4x4.CreateFromTranslation(vector3(-myManipulation.Translation.y, 0, 0))";
        animation->SetReferenceParameter("myManipulation", manipulation);
        visual->StartAnimation("TransformMatrix", animation);

        loadedEvent->Set();
    }));

    LOG_OUTPUT(L"Set WindowContent");
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    LOG_OUTPUT(L"Wait for Loaded event");
    loadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    auto pointerPressedRegistration = CreateSafeEventRegistration(UIElement, PointerPressed);
    auto pointerPressedEvent = std::make_shared<Event>();

    pointerPressedRegistration.Attach(
        border1,
        ref new PointerEventHandler(
        [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
    {
        LOG_OUTPUT(L"PointerPressed event received");
        pointerPressedEvent->Set();
    }));

    LOG_OUTPUT(L"ChangeView");
    RunOnUIThread([&]()
    {
        scrollViewer1->ChangeView(0.0, 200.0, 1.0f, true);
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tap and wait for PointerPressed event");
    TestServices::InputHelper->Tap(wf::Point(220, 150));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();
    TestServices::WindowHelper->WaitForIdle();

    // TODO:  This part of the test is susceptible to random timing problems
    // due to a known issue.  Re-enable this after the bug is fixed.
#if 0
    LOG_OUTPUT(L"Replace Content");
    RunOnUIThread([&]()
    {
        auto newContent = ref new Border();
        newContent->Height = 1000;
        newContent->Width = 300;
        newContent->Background = ref new SolidColorBrush(Colors::Gray);
        scrollViewer1->Content = newContent;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tap and wait for PointerPressed event");
    TestServices::InputHelper->Tap(wf::Point(220, 150));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();
    TestServices::WindowHelper->WaitForIdle();
#endif

    LOG_OUTPUT(L"Remove Content");
    RunOnUIThread([&]()
    {
        scrollViewer1->Content = nullptr;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Tap and wait for PointerPressed event");
    TestServices::InputHelper->Tap(wf::Point(20, 150));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();
    TestServices::WindowHelper->WaitForIdle();
}

namespace
{
    ref class ScrollViewerTestingPanel sealed : public Microsoft::UI::Xaml::Controls::Panel
    {
        bool m_hasMadeScrollAdjustment;

    protected:
        ScrollViewer^ GetAncestorScrollViewer()
        {
            for (DependencyObject ^dobj = this; dobj != nullptr; dobj = VisualTreeHelper::GetParent(dobj) )
            {
                try
                {
                    return safe_cast<Microsoft::UI::Xaml::Controls::ScrollViewer^>(dobj);
                }
                catch (Platform::Exception^)
                { }
            }
            return nullptr;
        }

        // Microsoft::UI::Xaml::IFrameworkElementOverrides overrides
        ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size /*availableSize*/) override
        {
            Microsoft::UI::Xaml::UIElement^ uie = Children->GetAt(0);
            uie->Measure(::Windows::Foundation::Size(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()));

            return ::Windows::Foundation::Size(400, 600);
        }

        ::Windows::Foundation::Size ArrangeOverride(::Windows::Foundation::Size /*finalSize*/) override
        {
            Microsoft::UI::Xaml::UIElement^ uie = Children->GetAt(0);
            uie->Arrange(::Windows::Foundation::Rect(0, 200, uie->DesiredSize.Width, uie->DesiredSize.Height));

            ScrollViewer ^scrollViewer = GetAncestorScrollViewer();
            if (!m_hasMadeScrollAdjustment && scrollViewer->VerticalOffset > 150)
            {
                LOG_OUTPUT(L"> Calling ChangeView from ArrangeOverride");
                m_hasMadeScrollAdjustment = true;
                scrollViewer->ChangeView(nullptr, scrollViewer->VerticalOffset - 100, nullptr, true);
            }

            return ::Windows::Foundation::Size(400, 600);
        }
    };
}

void local::XamlWinRTCompInteropManipTransformTests::ChangeViewDuringArrangeCommon(float expectedX)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& ih = TestServices::InputHelper;

    TestCleanupWrapper cleanup;
    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Grid ^rootPanel;
    ScrollViewer ^scrollViewer1;
    ScrollViewerTestingPanel ^panel1;
    Border ^border1;

    RunOnUIThread([&]()
    {
        border1 = ref new Border();
        border1->HorizontalAlignment = HorizontalAlignment::Left;
        border1->Height = 40;
        border1->Width = 40;
        border1->Background = ref new SolidColorBrush(Colors::Yellow);

        GradientStop^ stop1 = ref new GradientStop();
        stop1->Color = Microsoft::UI::Colors::Red;
        stop1->Offset = 0.45;

        GradientStop^ stop2 = ref new GradientStop();
        stop2->Color = Microsoft::UI::Colors::Green;
        stop2->Offset = 0.55;

        GradientStopCollection^ stopCollection = ref new GradientStopCollection();
        stopCollection->Append(stop1);
        stopCollection->Append(stop2);

        LinearGradientBrush^ brush = ref new LinearGradientBrush();
        brush->StartPoint = ::Windows::Foundation::Point(0, 0);
        brush->EndPoint = ::Windows::Foundation::Point(0, 1);
        brush->GradientStops = stopCollection;

        panel1 = ref new ScrollViewerTestingPanel();
        panel1->Background = brush;
        panel1->Children->Append(border1);

        scrollViewer1 = ref new ScrollViewer();
        scrollViewer1->Content = panel1;

        rootPanel = ref new Grid();
        rootPanel->Children->Append(scrollViewer1);
    });

    double currentScrollpos = 0;
    auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
    viewChangedRegistration.Attach(
        scrollViewer1,
        ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
        [&](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
    {
        LOG_OUTPUT(L"> ViewChanged raised.");
        if (currentScrollpos != scrollViewer1->VerticalOffset)
        {
            currentScrollpos = scrollViewer1->VerticalOffset;
            panel1->InvalidateMeasure();
        }
    }));

    auto loadedRegistration = CreateSafeEventRegistration(Grid, Loaded);
    auto loadedEvent = std::make_shared<Event>();

    ExpressionAnimation ^animation;

    loadedRegistration.Attach(
        rootPanel,
        ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
    {
        LOG_OUTPUT(L"> Loaded event received");

        Visual ^visual = ElementCompositionPreview::GetElementVisual(border1);

        // Bind Offset.y to compensate vertical scrolling so the object does not move vertically
        animation = visual->Compositor->CreateExpressionAnimation();
        animation->Expression = L"Matrix4x4.CreateFromTranslation(vector3(-myManipulation.Translation.y, -myManipulation.Translation.y, 0))";
        animation->SetReferenceParameter("myManipulation", ElementCompositionPreview::GetScrollViewerManipulationPropertySet(scrollViewer1));
        visual->StartAnimation("TransformMatrix", animation);

        loadedEvent->Set();
    }));

    LOG_OUTPUT(L"> Set WindowContent");
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootPanel;
    });

    LOG_OUTPUT(L"> Wait for Loaded event");
    loadedEvent->WaitForDefault();
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Register for PointerPressed events");
    auto targetPointerPressedRegistration = CreateSafeEventRegistration(UIElement, PointerPressed);
    auto pointerPressedEvent = std::make_shared<Event>();

    bool isHit = false;

    targetPointerPressedRegistration.Attach(
        border1,
        ref new PointerEventHandler(
        [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
    {
        LOG_OUTPUT(L"> target PointerPressed event received");
        isHit = true;
        args->Handled = true;
        pointerPressedEvent->Set();
    }));

    ih->PanFromCenter(scrollViewer1, 0, -150, 0.3);
    wh->WaitForIdle();

    // Keep trying until target is hit; there is no way to explicitly wait until manip animation completes
    for (int i = 0; i < 5; ++i)
    {
        LOG_OUTPUT(L"> Tap and wait for PointerPressed event");

        // Make sure the square is in the expected position. The ChangeView from Arrange should have stacked with the offset
        // from the pan. If the manipulation transform does not account for the adjustment, the yellow square will be off by
        // 100 pixels.
        ih->Tap(wf::Point(expectedX, 220));
        wh->WaitForIdle();
        pointerPressedEvent->WaitForNoThrow(std::chrono::milliseconds(500));

        if (isHit)
        {
            break;
        }

        pointerPressedEvent->Reset();
    };

    VERIFY_IS_TRUE(isHit, L"> Should be able to hit target in 5 attempts");
}

void local::XamlWinRTCompInteropManipTransformTests::ChangeViewDuringArrange_Desktop()
{
    // The same pan input lands in different places on desktop and phone, because the timing with Arrange/ChangeView is different.
    ChangeViewDuringArrangeCommon(80.0f);
}

void local::XamlWinRTCompInteropManipTransformTests::ChangeViewDuringArrange_Phone()
{
    // The same pan input lands in different places on desktop and phone, because the timing with Arrange/ChangeView is different.
    ChangeViewDuringArrangeCommon(100.0f);
}
