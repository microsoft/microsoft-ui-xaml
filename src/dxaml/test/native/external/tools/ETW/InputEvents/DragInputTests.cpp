// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DragInputTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include "TraceConsumerSession.h"
#include "MUX-ETWEvents.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::ApplicationModel::DataTransfer;
using namespace ::Windows::ApplicationModel::DataTransfer::DragDrop;
using namespace ::Windows::ApplicationModel::DataTransfer::DragDrop::Core;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace InputEvents {

    bool DragInputTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool DragInputTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //------------------------------------------------------------------------
    // Test case: Drag a Rectangle in a Canvas with the left mouse button.
    //------------------------------------------------------------------------
    void DragInputTests::DragEvents()
    {
        TestCleanupWrapper cleanup;

        std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> rectDropEvent = std::make_shared<Event>();

        Canvas^ canvas = nullptr;
        Shapes::Rectangle^ rect = nullptr;

        auto rectLoadedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Loaded);
        auto rectPointerPressedRegistration = CreateSafeEventRegistration(Shapes::Rectangle, PointerPressed);
        auto rectDragEnterRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragEnter);
        auto rectDragOverRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragOver);
        auto rectDragLeaveRegistration = CreateSafeEventRegistration(Shapes::Rectangle, DragLeave);
        auto rectDropRegistration = CreateSafeEventRegistration(Shapes::Rectangle, Drop);

        RunOnUIThread([&]()
        {
            Grid^ mainGrid = ref new Grid();

            canvas = ref new Canvas();
            canvas->Width = 300;
            canvas->Height = 400;
            canvas->HorizontalAlignment = HorizontalAlignment::Center;
            canvas->VerticalAlignment = VerticalAlignment::Center;
            canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
            mainGrid->Children->Append(canvas);

            rect = ref new Shapes::Rectangle();
            rect->Width = 200;
            rect->Height = 60;
            rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            rect->AllowDrop = true;
            canvas->Children->Append(rect);

            rectLoadedRegistration.Attach(rect, ref new RoutedEventHandler([rectLoadedEvent](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Rectangle Loaded event");
                rectLoadedEvent->Set();
            }));

            rectPointerPressedRegistration.Attach(rect, ref new PointerEventHandler([](Platform::Object^ sender, PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Rectangle PointerPressed event - Start Drag");
                CoreDragOperation^ drag = ref new CoreDragOperation();
                drag->SetPointerId(args->Pointer->PointerId);
                drag->StartAsync();
            }));

            rectDragEnterRegistration.Attach(rect, ref new DragEventHandler([](Platform::Object^ sender, DragEventArgs^ args)
            {
                LOG_OUTPUT(L"Rectangle DragEnter event");
            }));

            rectDragOverRegistration.Attach(rect, ref new DragEventHandler([](Platform::Object^ sender, DragEventArgs^ args)
            {
                LOG_OUTPUT(L"Rectangle DragOver event");
            }));

            rectDragLeaveRegistration.Attach(rect, ref new DragEventHandler([](Platform::Object^ sender, DragEventArgs^ args)
            {
                LOG_OUTPUT(L"Rectangle DragLeave event");
            }));

            rectDropRegistration.Attach(rect, ref new DragEventHandler([rectDropEvent](Platform::Object^ sender, DragEventArgs^ args)
            {
                LOG_OUTPUT(L"Rectangle Drop event");
                rectDropEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = mainGrid;
        });
        TraceConsumerSession traceSession(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);


        TraceConsumer::EnableTracingByEventId(DragEnterBegin_value);
        TraceConsumer::EnableTracingByEventId(DragEnterEnd_value);
        TraceConsumer::EnableTracingByEventId(DragLeaveBegin_value);
        TraceConsumer::EnableTracingByEventId(DragLeaveEnd_value);
        TraceConsumer::EnableTracingByEventId(DragOverBegin_value);
        TraceConsumer::EnableTracingByEventId(DragOverEnd_value);
        TraceConsumer::EnableTracingByEventId(DropBegin_value);
        TraceConsumer::EnableTracingByEventId(DropEnd_value);

        TestServices::WindowHelper->WaitForIdle();
        rectLoadedEvent->WaitForDefault();
        LOG_OUTPUT(L"Dragging Rectangle.");
        TestServices::InputHelper->DragFromCenter(rect, 50 /*relX*/, 0 /*relY*/, 0.1 /*velocityFactor*/);
        rectDropEvent->WaitForDefault();

        traceSession.Stop();

        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DragEnterBegin_value, 1));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DragEnterEnd_value, 1));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DragLeaveBegin_value, 0));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DragLeaveEnd_value, 0));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DragOverBegin_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DragOverEnd_value));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DropBegin_value, 1));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(DropEnd_value, 1));

} } } } } } } }