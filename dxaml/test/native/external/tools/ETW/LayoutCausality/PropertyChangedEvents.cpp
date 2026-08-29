// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <FileLoader.h>
#include <TreeHelper.h>
#include <CustomPropertySupport.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include "PropertyChangedEvents.h"
#include "TraceConsumerSession.h"
#include "MUX-ETWEvents.h"
#include <TestEvent.h>
#include <SafeEventRegistration.h>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
namespace Tools { namespace ETW { namespace LayoutCausality {


    bool PropertyChangedEventTests::ClassSetup()
    {
        // It's very important to call EnsureInitialized on TestServices
        // from ClassSetup. This method will wait for the window to be
        // activated on launch, which avoids a race condition that will block
        // input from being routed to the app. It will also wait for the
        // debugger to attach when the waitForDebugger runtime parameter is
        // specified.
        CommonTestSetupHelper::CommonTestClassSetup();

        return true;
    }

    bool PropertyChangedEventTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }


    void PropertyChangedEventTests::UpdateAffectsLayoutOnly()
    {
        TestCleanupWrapper cleanup;

        Grid^ mainGrid = nullptr;
        Shapes::Rectangle^ rect = nullptr;

        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        auto loaded = CreateSafeEventRegistration(FrameworkElement, Loaded);

        RunOnUIThread([&]()
        {
            mainGrid = ref new Grid();

            mainGrid->Width = 300;
            mainGrid->Height = 400;
            mainGrid->VerticalAlignment = VerticalAlignment::Center;
            mainGrid->HorizontalAlignment = HorizontalAlignment::Center;
            mainGrid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
            TestServices::WindowHelper->WindowContent = mainGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rect = ref new Shapes::Rectangle();
            rect->Height = 100;
            rect->Width = 100;
            rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            loaded.Attach(
                rect,
                ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            }));
            mainGrid->Children->Append(rect);

        });

        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();


        // Start the tracing session now that rectangle is in live tree
        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(PropertyChangedInfo_value);
        TraceConsumer::EnableTracingByEventId(InvalidateArrangeInfo_value);
        TraceConsumer::EnableTracingByEventId(InvalidateMeasureInfo_value);

        // Change properties on the rectangle and verify correct events fire
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing property on rectangle");
            rect->Height = 300;
        });

        TestServices::WindowHelper->WaitForIdle();
        session.Stop();

        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(PropertyChangedInfo_value, 1));
        // I am not sure why this is two, I would expect one for the size property changed.
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(InvalidateArrangeInfo_value, 2));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(InvalidateMeasureInfo_value, 2));
    }

    void PropertyChangedEventTests::UpdateAffectsRenderOnly()
    {
        TestCleanupWrapper cleanup;

        Grid^ mainGrid = nullptr;
        Shapes::Rectangle^ rect = nullptr;

        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        auto loaded = CreateSafeEventRegistration(FrameworkElement, Loaded);

        RunOnUIThread([&]()
        {
            mainGrid = ref new Grid();

            mainGrid->Width = 300;
            mainGrid->Height = 400;
            mainGrid->VerticalAlignment = VerticalAlignment::Center;
            mainGrid->HorizontalAlignment = HorizontalAlignment::Center;
            mainGrid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
            TestServices::WindowHelper->WindowContent = mainGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rect = ref new Shapes::Rectangle();
            rect->Height = 100;
            rect->Width = 100;
            rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            loaded.Attach(
                rect,
                ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            }));
            mainGrid->Children->Append(rect);

        });

        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();


        // Start the tracing session now that rectangle is in live tree
        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(PropertyChangedInfo_value);
        TraceConsumer::EnableTracingByEventId(InvalidateArrangeInfo_value);
        TraceConsumer::EnableTracingByEventId(InvalidateMeasureInfo_value);

        // Change properties on the rectangle and verify correct events fire
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing property on rectangle");
            rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        });

        TestServices::WindowHelper->WaitForIdle();
        session.Stop();

        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(PropertyChangedInfo_value, 1));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(InvalidateArrangeInfo_value, 0));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(InvalidateMeasureInfo_value, 0));

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }
    void PropertyChangedEventTests::UpdateDoesntAffectLayout()
    {
        TestCleanupWrapper cleanup;

        Grid^ mainGrid = nullptr;
        Shapes::Rectangle^ rect = nullptr;

        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        auto loaded = CreateSafeEventRegistration(FrameworkElement, Loaded);

        RunOnUIThread([&]()
        {
            mainGrid = ref new Grid();

            mainGrid->Width = 300;
            mainGrid->Height = 400;
            mainGrid->VerticalAlignment = VerticalAlignment::Center;
            mainGrid->HorizontalAlignment = HorizontalAlignment::Center;
            mainGrid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
            TestServices::WindowHelper->WindowContent = mainGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rect = ref new Shapes::Rectangle();
            rect->Height = 100;
            rect->Width = 100;
            rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            loaded.Attach(
                rect,
                ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            }));
            mainGrid->Children->Append(rect);

        });

        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();

        // Start the tracing session now that rectangle is in live tree
        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(PropertyChangedInfo_value);
        TraceConsumer::EnableTracingByEventId(InvalidateArrangeInfo_value);
        TraceConsumer::EnableTracingByEventId(InvalidateMeasureInfo_value);

        // Change properties on the rectangle and verify correct events fire
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing property on rectangle");
            rect->AllowDrop = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        session.Stop();

        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(PropertyChangedInfo_value, 0));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(InvalidateArrangeInfo_value, 0));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(InvalidateMeasureInfo_value, 0));

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }
    void PropertyChangedEventTests::DirtyElementFiresEvent()
    {
        TestCleanupWrapper cleanup;

        Grid^ mainGrid = nullptr;
        Shapes::Rectangle^ rect = nullptr;

        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        auto loaded = CreateSafeEventRegistration(FrameworkElement, Loaded);

        RunOnUIThread([&]()
        {
            mainGrid = ref new Grid();

            mainGrid->Width = 300;
            mainGrid->Height = 400;
            mainGrid->VerticalAlignment = VerticalAlignment::Center;
            mainGrid->HorizontalAlignment = HorizontalAlignment::Center;
            mainGrid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
            TestServices::WindowHelper->WindowContent = mainGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rect = ref new Shapes::Rectangle();
            rect->Height = 100;
            rect->Width = 100;
            rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            loaded.Attach(
                rect,
                ref new xaml::RoutedEventHandler([&rectLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            }));
            mainGrid->Children->Append(rect);

        });

        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();

        // Start the tracing session now that rectangle is in live tree
        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(PropertyChangedInfo_value);
        TraceConsumer::EnableTracingByEventId(InvalidateArrangeInfo_value);
        TraceConsumer::EnableTracingByEventId(InvalidateMeasureInfo_value);

        // Change properties on the rectangle and verify correct events fire
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing property on rectangle");
            rect->Height = 300;
            LOG_OUTPUT(L"Changing it again!");
            rect->Height = 250;
        });

        TestServices::WindowHelper->WaitForIdle();
        session.Stop();

        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(PropertyChangedInfo_value, 2));
        // This should still be two for Invalidation (which is the count when only one property changed
        // and means that the layout wasn't affected twice.
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(InvalidateArrangeInfo_value, 2));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(InvalidateMeasureInfo_value, 2));

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }
} } } } } } }