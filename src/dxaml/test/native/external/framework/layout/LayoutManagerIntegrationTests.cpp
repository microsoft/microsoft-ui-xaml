// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "LayoutManagerIntegrationTests.h"

#include <Layout.CustomTypes.h>
#include <DisableErrorReportingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace test_infra;
using namespace ::Tests::Native::External::Framework::Layout;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layout {

    ref class InvalidateViewportHelper : public xaml::FrameworkElement
    {
        public:
        // InvalidateViewport is protected. So in order to call it we create a subclass
        // of FrameworkElement and invoke InvalidateViewport from there
        static void InvalidateViewport(xaml::FrameworkElement^ element)
        {
            element->InvalidateViewport();
        }
    };
    bool LayoutManagerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool LayoutManagerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool LayoutManagerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void LayoutManagerIntegrationTests::ValidateEffectiveViewportChanged()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        bool eventFired = false;
        ViewportUserControl^ vuc = nullptr;
        CompositeTransform^ transform = nullptr;
        Border^ b0 = nullptr;
        Border^ b1 = nullptr;
        Border^ b2 = nullptr;
        Border^ b3 = nullptr;
        Border^ b4 = nullptr;
        Border^ b5 = nullptr;

        auto eventReg0 = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);
        auto eventReg1 = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);
        auto eventReg2 = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);
        auto eventReg3 = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);
        auto eventReg4 = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);
        auto eventReg5 = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);

        LOG_OUTPUT(L"Build a new visual tree and verify that the event fires with the correct values.");
        RunOnUIThread([&]()
        {
            b0 = ref new Border;
            b0->Height = 50;
            b0->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

            b1 = ref new Border;
            b1->Height = 50;
            b1->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

            b2 = ref new Border;
            b2->Height = 50;
            b2->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

            b3 = ref new Border;
            b3->Height = 50;
            b3->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

            b4 = ref new Border;
            b4->Height = 50;
            b4->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

            b5 = ref new Border;
            b5->Height = 50;
            b5->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

            transform = ref new CompositeTransform;

            auto sp = ref new StackPanel;
            sp->Width = 100;
            sp->RenderTransform = transform;
            sp->RenderTransformOrigin = ::Windows::Foundation::Point(0.5, 0.0);

            vuc = ref new ViewportUserControl;
            vuc->Width = 100;
            vuc->Height = 100;

            auto c = ref new Canvas;

            auto g = ref new Grid;
            g->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);

            sp->Children->Append(b0);
            sp->Children->Append(b1);
            sp->Children->Append(b2);
            sp->Children->Append(b3);
            sp->Children->Append(b4);
            sp->Children->Append(b5);
            c->Children->Append(sp);
            vuc->Content = c;
            g->Children->Append(vuc);
            TestServices::WindowHelper->WindowContent = g;

            eventReg0.Attach(
                safe_cast<xaml::FrameworkElement^>(b0),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [&eventFired](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                eventFired = true;
                VERIFY_ARE_EQUAL(0.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(0.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Height);
            }));

            eventReg1.Attach(
                safe_cast<xaml::FrameworkElement^>(b1),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(0.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-50.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Height);
            }));

            eventReg2.Attach(
                safe_cast<xaml::FrameworkElement^>(b2),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(0.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-100.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Height);
            }));

            eventReg3.Attach(
                safe_cast<xaml::FrameworkElement^>(b3),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(0.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-150.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Height);
            }));

            eventReg4.Attach(
                safe_cast<xaml::FrameworkElement^>(b4),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(0.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-200.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Height);
            }));

            eventReg5.Attach(
                safe_cast<xaml::FrameworkElement^>(b5),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(0.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-250.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(100.0f, e->EffectiveViewport.Height);
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(eventFired);

        LOG_OUTPUT(L"Verify that the event does not fire if values do not change, even when the viewport is explicitly invalidated.");
        eventFired = false;
        RunOnUIThread([&]()
        {
            InvalidateViewportHelper::InvalidateViewport(vuc);

            eventReg0.Detach();
            eventReg1.Detach();
            eventReg2.Detach();
            eventReg3.Detach();
            eventReg4.Detach();
            eventReg5.Detach();

            eventReg0.Attach(
                safe_cast<xaml::FrameworkElement^>(b0),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [&eventFired](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                eventFired = true;
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(eventFired);

        LOG_OUTPUT(L"Verify that the event fires again with the correct values after modifying translations and scales.");
        RunOnUIThread([&]()
        {
            transform->TranslateY = 50.0;
            transform->ScaleX = 2.0;
            transform->ScaleY = 2.0;
            InvalidateViewportHelper::InvalidateViewport(vuc);

            eventReg0.Detach();
            eventReg0.Attach(
                safe_cast<xaml::FrameworkElement^>(b0),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [&eventFired](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                eventFired = true;
                VERIFY_ARE_EQUAL(25.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-25.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Height);
            }));

            eventReg1.Attach(
                safe_cast<xaml::FrameworkElement^>(b1),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(25.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-75.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Height);
            }));

            eventReg2.Attach(
                safe_cast<xaml::FrameworkElement^>(b2),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(25.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-125.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Height);
            }));

            eventReg3.Attach(
                safe_cast<xaml::FrameworkElement^>(b3),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(25.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-175.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Height);
            }));

            eventReg4.Attach(
                safe_cast<xaml::FrameworkElement^>(b4),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(25.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-225.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Height);
            }));

            eventReg5.Attach(
                safe_cast<xaml::FrameworkElement^>(b5),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(25.0f, e->EffectiveViewport.X);
                VERIFY_ARE_EQUAL(-275.0f, e->EffectiveViewport.Y);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Width);
                VERIFY_ARE_EQUAL(50.0f, e->EffectiveViewport.Height);
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(eventFired);
    }

    void LayoutManagerIntegrationTests::ValidateEffectiveViewportChangedUnregistration()
    {
        TestCleanupWrapper cleanup;
        ViewportUserControl^ vuc = nullptr;
        auto eventReg0 = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);
        auto eventReg1 = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);

        LOG_OUTPUT(L"Build a new visual tree and attach two EffectiveViewportChanged events on the same target.");
        RunOnUIThread([&]()
        {
            auto b = ref new Border;
            b->Height = 50;
            b->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

            vuc = ref new ViewportUserControl;
            vuc->Width = 100;
            vuc->Height = 100;

            auto g = ref new Grid;

            vuc->Content = b;
            g->Children->Append(vuc);
            TestServices::WindowHelper->WindowContent = g;

            eventReg0.Attach(
                safe_cast<xaml::FrameworkElement^>(b),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                // This event does nothing, but we need to attach it.
                // Otherwise, the effective viewport walk won't occur.
            }));

            eventReg1.Attach(
                safe_cast<xaml::FrameworkElement^>(b),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                // This event does nothing, but we need to attach it.
                // Otherwise, the effective viewport walk won't occur.
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Detach one of the events and invalidate the viewport.");
        RunOnUIThread([&]()
        {
            eventReg1.Detach();
            InvalidateViewportHelper::InvalidateViewport(vuc);
        });
        TestServices::WindowHelper->WaitForIdle();

        // We're clearing the LF_WANTS_VIEWPORT flag on the target, but we
        // go ahead and clear the LF_CONTRIBUTES_TO_VIEWPORT flag on the
        // ancestors. During the next effective viewport walk, this flag will
        // be cleared and not restored.
        LOG_OUTPUT(L"Detach the other event (which cleans the target but not the ancestors) and invalidate the viewport.");
        RunOnUIThread([&]()
        {
            eventReg0.Detach();
            InvalidateViewportHelper::InvalidateViewport(vuc);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invalidate the viewport once more.");
        RunOnUIThread([&]()
        {
            InvalidateViewportHelper::InvalidateViewport(vuc);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void LayoutManagerIntegrationTests::ThrowsExceptionOnInvalidateViewportForNonScrollers()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            // Calling InvalidateViewport on an element that has been
            // registered as a scroller should succeed.
            ViewportUserControl^ scroller = ref new ViewportUserControl;
            InvalidateViewportHelper::InvalidateViewport(scroller);

            // Calling InvalidateViewport on an element that has *not* been
            // registered as a scroller should throw an exception.
            Border^ nonScroller = ref new Border;
            VERIFY_THROWS_WINRT(
                InvalidateViewportHelper::InvalidateViewport(nonScroller),
                Platform::Exception^);
        });
    }

    void LayoutManagerIntegrationTests::ThrowsExceptionIfLayoutIsInvalidatedDuringViewportWalk()
    {
        TestCleanupWrapper cleanup;
        ViewportUserControl^ vuc = nullptr;
        auto eventReg = CreateSafeEventRegistration(xaml::FrameworkElement, EffectiveViewportChanged);

        // Ensure the validity of this scenario by letting the LayoutManager
        // run normally once.
        RunOnUIThread([&]()
        {
            auto b = ref new Border;
            b->Height = 50;
            b->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

            vuc = ref new ViewportUserControl;
            vuc->Width = 100;
            vuc->Height = 100;

            auto g = ref new Grid;

            vuc->Content = b;
            g->Children->Append(vuc);
            TestServices::WindowHelper->WindowContent = g;

            eventReg.Attach(
                safe_cast<xaml::FrameworkElement^>(b),
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, xaml::EffectiveViewportChangedEventArgs^>(
                    [](Platform::Object^ sender, xaml::EffectiveViewportChangedEventArgs^ e)
            {
                // This event does nothing, but we need to attach it.
                // Otherwise, the effective viewport walk won't occur.
            }));
        });
        TestServices::WindowHelper->WaitForIdle();
    }


} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layout