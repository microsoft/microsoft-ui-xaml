// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CompositionTargetTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Maps;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool CompositionTargetTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool CompositionTargetTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool CompositionTargetTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void CompositionTargetTests::RenderingEvent1()
{
    // Synopsis:
    // Register one listener for CompositionTarget.Rendering event
    // Validate the event fires 5 times
    // Unregister
    // Validate XAML stops scheduling ticks
    const auto& wh = TestServices::WindowHelper;
    ::Windows::Foundation::EventRegistrationToken renderingEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderingEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    RunOnUIThread([&]()
    {
        Canvas^ rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    std::shared_ptr<Event> completionEvent = std::make_shared<Event>();
    int eventCount = 0;

    RunOnUIThread([&]()
    {
        auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendering]");
            eventCount++;
            if (eventCount == 2)
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
                completionEvent->Set();
            }
        });

        LOG_OUTPUT(L"Registering for Rendering event");
        renderingEventToken = CompositionTarget::Rendering::add(onRendering);
    });
    completionEvent->WaitForDefault();
    VERIFY_IS_FALSE(wh->GetWantsRenderingEvent());
}

void CompositionTargetTests::RenderingEvent2()
{
    // Synopsis:
    // Register two listeners for CompositionTarget.Rendering event
    // Validate the first event fires 5 times, and the second event fires 10 times
    // Unregister listeners as they reach their validation points
    // Validate XAML stops scheduling ticks
    const auto& wh = TestServices::WindowHelper;
    ::Windows::Foundation::EventRegistrationToken renderingEventToken1 = {};
    ::Windows::Foundation::EventRegistrationToken renderingEventToken2 = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderingEventToken1.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken1);
            });
        }

        if (renderingEventToken2.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken2);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    RunOnUIThread([&]()
    {
        Canvas^ rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    std::shared_ptr<Event> completionEvent = std::make_shared<Event>();
    int eventCount1 = 0;
    int eventCount2 = 0;

    RunOnUIThread([&]()
    {
        auto onRendering1 = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendering] (1)");
            eventCount1++;
            if (eventCount1 == 2)
            {
                CompositionTarget::Rendering::remove(renderingEventToken1);
            }
        });

        auto onRendering2 = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendering] (2)");
            eventCount2++;
            if (eventCount2 == 4)
            {
                CompositionTarget::Rendering::remove(renderingEventToken2);
                completionEvent->Set();
            }
        });

        LOG_OUTPUT(L"Registering for Rendering event2");
        renderingEventToken1 = CompositionTarget::Rendering::add(onRendering1);
        renderingEventToken2 = CompositionTarget::Rendering::add(onRendering2);
    });
    completionEvent->WaitForDefault();
    VERIFY_IS_FALSE(wh->GetWantsRenderingEvent());
}

void CompositionTargetTests::RenderingEvent3()
{
    // Synopsis:
    // Register one listener for CompositionTarget.Rendering event
    // Validate the event fires 5 times
    // Unregister by returning RPC_E_DISCONNECTED in event handler
    // Validate XAML stops scheduling ticks
    const auto& wh = TestServices::WindowHelper;
    ::Windows::Foundation::EventRegistrationToken renderingEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderingEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    RunOnUIThread([&]()
    {
        Canvas^ rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    std::shared_ptr<Event> completionEvent = std::make_shared<Event>();
    int eventCount = 0;

    RunOnUIThread([&]()
    {
        auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendering]");
            eventCount++;
            if (eventCount == 2)
            {
                completionEvent->Set();
                throw ref new Platform::COMException(RPC_E_DISCONNECTED);
            }
        });

        LOG_OUTPUT(L"Registering for Rendering event");
        renderingEventToken = CompositionTarget::Rendering::add(onRendering);
    });
    completionEvent->WaitForDefault();
    VERIFY_IS_FALSE(wh->GetWantsRenderingEvent());
}

void CompositionTargetTests::RenderedEventShouldNotRequestFrame()
{
    // Synopsis:
    // Register one listener for CompositionTarget.Rendered event
    // Create an empty canvas
    // Validate registering a handler for 'Rendered' will not request a frame (force rendering)
    // The event should not fire unless something else requires a frame
    const auto& wh = TestServices::WindowHelper;
    ::Windows::Foundation::EventRegistrationToken renderedEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderedEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendered::remove(renderedEventToken);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    // Root canvas used as WindowContent
    Canvas^ rootCanvas = nullptr;

    // Set empty Canvas to WindowContent
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting WindowContent...");
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    // When CompositionIslands are in use, an extra frame is typically produced due to an asynchronous notification
    // which changes the CompositionIsland Visibility to Visible.  It would be difficult to teach WaitForIdle how
    // to wait for this, so workaround by ticking the UI thread one more time.
    // App activation can also cause additional frames. Tick the UI thread a few more times to skip those, too.
    wh->SynchronouslyTickUIThread(5);

    // Event signalled by test event handler when 'Rendered' fires
    std::shared_ptr<Event> completionEvent = std::make_shared<Event>();

    // Number of times the event handler has been called in response to 'Rendered' event
    int eventCount = 0;

    // Hook up test event handler to 'Rendered' event
    RunOnUIThread([&]()
    {
        auto onRendered = ref new ::Windows::Foundation::EventHandler<Microsoft::UI::Xaml::Media::RenderedEventArgs^>([&](Platform::Object^ sender, Microsoft::UI::Xaml::Media::RenderedEventArgs^ renderedEventArgs)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendered]");
            eventCount++;

            LOG_OUTPUT(L"[CompositionTarget.Rendered]: FrameDuration in 100ns units: %d", renderedEventArgs->FrameDuration.Duration);
            CompositionTarget::Rendered::remove(renderedEventToken);
            completionEvent->Set();
        });

        LOG_OUTPUT(L"Registering for Rendered event");
        renderedEventToken = CompositionTarget::Rendered::add(onRendered);
    });

    // Allow the event to time out
    completionEvent->WaitForNoThrow(std::chrono::milliseconds(100));

    // Verify the event did not fire
    VERIFY_IS_FALSE(completionEvent->HasFired());

    // Confirm the event handler is still hooked up
    VERIFY_IS_TRUE(wh->GetWantsCompositionTargetRenderedEvent());

    // Verify the event count is 0 : no frames rendered, no event callbacks
    VERIFY_ARE_EQUAL(eventCount, 0);
}
void CompositionTargetTests::RenderedEvent1()
{
    // Synopsis:
    // Register one listener for CompositionTarget.Rendered event
    // Force a frame
    // Validate the event fires
    // Validate CCoreServices 'm_fWantsCompositionTargetRenderedEvent' flag is false after removing handler
    // Force another frame
    // Verify that the test listener is not called back
    const auto& wh = TestServices::WindowHelper;
    ::Windows::Foundation::EventRegistrationToken renderedEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderedEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendered::remove(renderedEventToken);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    // Canvas used as root WindowContent
    Canvas^ rootCanvas = nullptr;
    TextBlock^ textBlock = nullptr;

    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    // Event signalled by test event handler when 'Rendered' fires
    std::shared_ptr<Event> completionEvent = std::make_shared<Event>();

    // Number of times the event handler has been called in response to 'Rendered' event
    int eventCount = 0;

    // Hook up test event handler to 'Rendered' event
    RunOnUIThread([&]()
    {
        auto onRendered = ref new ::Windows::Foundation::EventHandler<Microsoft::UI::Xaml::Media::RenderedEventArgs^>([&](Platform::Object^ sender, Microsoft::UI::Xaml::Media::RenderedEventArgs^ renderedEventArgs)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendered] : inside test 'Rendered' handler...");
            eventCount++;

            LOG_OUTPUT(L"[CompositionTarget.Rendered]: FrameDuration in 100ns units: %d", renderedEventArgs->FrameDuration.Duration);

            CompositionTarget::Rendered::remove(renderedEventToken);
            completionEvent->Set();
        });

        LOG_OUTPUT(L"Registering for Rendered event");
        renderedEventToken = CompositionTarget::Rendered::add(onRendered);

        LOG_OUTPUT(L"Adding content to force rendering...");
        textBlock = ref new TextBlock();
        textBlock->Text = L"This is a text block";
        rootCanvas->Children->Append(textBlock);
    });

    // Wait for the test handler to signal in response to a CompositionTarget.Rendered event
    completionEvent->WaitForDefault();

    // Verify that no handlers are registered on 'Rendered'
    // These should have been removed inside the test callback
    VERIFY_IS_FALSE(wh->GetWantsCompositionTargetRenderedEvent());

    // Verify the event count is 1
    VERIFY_ARE_EQUAL(eventCount, 1);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Adding content to force rendering a second frame: the event handler should be removed so the callback should no longer fire...");
        textBlock = ref new TextBlock();
        textBlock->Text = L"This is a text block";
        rootCanvas->Children->Append(textBlock);
    });

    wh->WaitForIdle();

    // Verify that no handlers are registered on 'Rendered'
    // These should have been removed inside the test callback
    VERIFY_IS_FALSE(wh->GetWantsCompositionTargetRenderedEvent());

    // Verify the event count is 1
    VERIFY_ARE_EQUAL(eventCount, 1);
}

void CompositionTargetTests::RenderedEvent2()
{
    // Synopsis:
    // Register two listeners for the CompositionTarget.Rendered event
    // Force rendering 10 frames by modifying the tree
    // Unregister listener #1 after the 5th frame
    // Unregister listener #2 after the 10th frame
    // Verify callback counts and 'm_fWantsCompositionTargetRenderedEvent' flag on CCoreServices
    const auto& wh = TestServices::WindowHelper;

    // Event tokens for test handlers
    ::Windows::Foundation::EventRegistrationToken renderedEventToken1 = {};
    ::Windows::Foundation::EventRegistrationToken renderedEventToken2 = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderedEventToken1.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendered::remove(renderedEventToken1);
            });
        }

        if (renderedEventToken2.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendered::remove(renderedEventToken2);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    // Canvas used as root WindowContent
    Canvas^ rootCanvas = nullptr;

    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    // Event signalled by test event handler when 'Rendered' fires
    std::shared_ptr<Event> completionEvent1 = std::make_shared<Event>();
    std::shared_ptr<Event> completionEvent2 = std::make_shared<Event>();

    // Number of times the event handler has been called in response to 'Rendered' event
    int eventCount1 = 0;
    int eventCount2 = 0;

    // Hook up test event handler to 'Rendered' event
    RunOnUIThread([&]()
    {
        auto onRendered1 = ref new ::Windows::Foundation::EventHandler<Microsoft::UI::Xaml::Media::RenderedEventArgs^>([&](Platform::Object^ sender, Microsoft::UI::Xaml::Media::RenderedEventArgs^ renderedEventArgs)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendered] #1 : inside test 'Rendered' handler #1 ...");

            eventCount1++;
            LOG_OUTPUT(L"[CompositionTarget.Rendered] #1 : eventCount1: %d", eventCount1);

            LOG_OUTPUT(L"[CompositionTarget.Rendered] #1: FrameDuration in 100ns units: %d", renderedEventArgs->FrameDuration.Duration);

            if (eventCount1 == 5)
            {
                CompositionTarget::Rendered::remove(renderedEventToken1);
                completionEvent1->Set();
            }
        });

        auto onRendered2 = ref new ::Windows::Foundation::EventHandler<Microsoft::UI::Xaml::Media::RenderedEventArgs^>([&](Platform::Object^ sender, Microsoft::UI::Xaml::Media::RenderedEventArgs^ renderedEventArgs)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendered] (2) : inside test 'Rendered' handler #2 ...");

            eventCount2++;
            LOG_OUTPUT(L"[CompositionTarget.Rendered] #2 : eventCount2: %d", eventCount2);

            LOG_OUTPUT(L"[CompositionTarget.Rendered] #2: FrameDuration in 100ns units: %d", renderedEventArgs->FrameDuration.Duration);

            if (eventCount2 == 10)
            {
                CompositionTarget::Rendered::remove(renderedEventToken2);
                completionEvent2->Set();
            }
        });

        LOG_OUTPUT(L"Registering for Rendered event #1");
        renderedEventToken1 = CompositionTarget::Rendered::add(onRendered1);

        LOG_OUTPUT(L"Registering for Rendered event #2");
        renderedEventToken2 = CompositionTarget::Rendered::add(onRendered2);
    });

    for (int i = 0; i < 10; ++i)
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Adding content to force rendering...");
            TextBlock^ textBlocks = ref new TextBlock();
            textBlocks->Text = L"This is a text block";
            rootCanvas->Children->Append(textBlocks);
        });

        wh->WaitForIdle();
    }

    LOG_OUTPUT(L"Wait for the test handler to signal in response to a CompositionTarget.Rendered event from handler #1...");
    completionEvent1->WaitForDefault();

    LOG_OUTPUT(L"Wait for the test handler to signal in response to a CompositionTarget.Rendered event from handler #2...");
    completionEvent2->WaitForDefault();

    LOG_OUTPUT(L"Verify that no handlers are registered on 'Rendered'");
    // These should have been removed inside the test callback
    VERIFY_IS_FALSE(wh->GetWantsCompositionTargetRenderedEvent());

    LOG_OUTPUT(L"Verify the event count for first handler");
    VERIFY_ARE_EQUAL(eventCount1, 5);

    LOG_OUTPUT(L"Verify the event count for second handler");
    VERIFY_ARE_EQUAL(eventCount2, 10);
}

void CompositionTargetTests::RenderedEvent3()
{
    // Synopsis:
    // Register one listener for CompositionTarget.Rendered event
    // Validate the event fires once
    // Unregister by returning RPC_E_DISCONNECTED in event handler
    // Validate CCoreServices 'm_fWantsCompositionTargetRenderedEvent' flag is false
    const auto& wh = TestServices::WindowHelper;
    ::Windows::Foundation::EventRegistrationToken renderedEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderedEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendered::remove(renderedEventToken);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    std::shared_ptr<Event> completionEvent = std::make_shared<Event>();
    int eventCount = 0;

    RunOnUIThread([&]()
    {
        auto onRendered = ref new ::Windows::Foundation::EventHandler<Microsoft::UI::Xaml::Media::RenderedEventArgs^>([&](Platform::Object^ sender, Microsoft::UI::Xaml::Media::RenderedEventArgs^ renderedEventArgs)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendered]");
            eventCount++;

            LOG_OUTPUT(L"[CompositionTarget.Rendered]: FrameDuration in 100ns units: %I64d", renderedEventArgs->FrameDuration.Duration);

            completionEvent->Set();
            throw ref new Platform::COMException(RPC_E_DISCONNECTED);
        });

        LOG_OUTPUT(L"Registering for Rendered event");
        renderedEventToken = CompositionTarget::Rendered::add(onRendered);
    });

    RunOnUIThread([&]()
    {
        Canvas^ rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    completionEvent->WaitForDefault();
    VERIFY_IS_FALSE(wh->GetWantsCompositionTargetRenderedEvent());
}

void CompositionTargetTests::RenderingDoesNotStarveInput()
{
    // Synopsis:
    // Register listener for CompositionTarget.Rendering event, request it over and over
    // Inject a click, make sure it makes it through the repeated CT.R events

    const auto& wh = TestServices::WindowHelper;
    const auto& ih = TestServices::InputHelper;

    ::Windows::Foundation::EventRegistrationToken renderingEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderingEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    Grid^ clickGrid;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree");
        clickGrid = ref new Grid();
        clickGrid->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        clickGrid->Width = 400;
        clickGrid->Height = 400;

        pointerPressedRegistration.Attach(
            clickGrid,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"> Hit Test succeeded. PointerPressed event received.");
            pointerPressedEvent->Set();
        }));

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(clickGrid);

        wh->WindowContent = rootCanvas;
    });
    wh->WaitForIdle();

    std::shared_ptr<Event> started = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"> CompositionTarget.Rendering");
            if (started != nullptr)
            {
                started->Set();
            }
        });

        LOG_OUTPUT(L"> Registering for Rendering event");
        renderingEventToken = CompositionTarget::Rendering::add(onRendering);
    });
    started->WaitForDefault();
    started = nullptr;

    LOG_OUTPUT(L"> Tapping on grid");
    ih->Tap(clickGrid);

    pointerPressedEvent->WaitForDefault();
}

} } } } } }
