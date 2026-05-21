// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::Media::Playback;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

class MediaEndedEvent
{
    Event event;
    ::Windows::Foundation::EventRegistrationToken registrationToken;
    ::Windows::Media::Playback::MediaPlayer^ mediaPlayer;

public:
    MediaEndedEvent(xaml_controls::MediaPlayerElement^ mediaPlayerElement) : mediaPlayer(nullptr)
    {
        RunOnUIThread([&]()
        {
            mediaPlayer = mediaPlayerElement->MediaPlayer;
            registrationToken =
                mediaPlayer->MediaEnded +=
                    ref new ::Windows::Foundation::TypedEventHandler<MediaPlayer^, Platform::Object ^>(
                        [&](MediaPlayer^, Platform::Object ^)
                        {
                            RunOnUIThread([&]()
                            {
                                LOG_OUTPUT(L"Media Ended Event Fired");
                                event.Set();
                            });
                        });
        });
    }

    ~MediaEndedEvent()
    {
        LOG_OUTPUT(L"Cleaning event handler");
        RunOnUIThread([&]()
        {
            mediaPlayer->MediaEnded -= registrationToken;
        });
    }

    void Wait()
    {
        LOG_OUTPUT(L"Waiting for media to complete");
        event.WaitFor(std::chrono::milliseconds(10*1000));
        LOG_OUTPUT(L"WaitForIdle");
        TestServices::WindowHelper->WaitForIdle();
    }
};

class DoubleTappedEventTester
{
    xaml::UIElement^ m_owner;
    Platform::Object^ m_lastSender;
    int m_executeCount;
    Event m_event;

public:
    DoubleTappedEventTester(xaml::UIElement^ owner)
        : m_owner(owner), m_executeCount(0), m_lastSender(nullptr)
    {
        RunOnUIThread([&]()
        {
            eventRegistration.Attach(
                m_owner,
                ref new xaml::Input::DoubleTappedEventHandler([&](Platform::Object^ sender, xaml::Input::DoubleTappedRoutedEventArgs^)
                {
                    m_lastSender = sender;
                    ++m_executeCount;
                    m_event.Set();
                }));
        });
    }

    int GetExecuteCount() const
    {
        return m_executeCount;
    }

    Platform::Object^ GetLastSender() const
    {
        return m_lastSender;
    }

    void Wait()
    {
        LOG_OUTPUT(L"WaitForIdle");
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for event to fire");
        m_event.WaitForDefault();
        LOG_OUTPUT(L"WaitForIdle");
        TestServices::WindowHelper->WaitForIdle();
    }

private:
    SafeEventRegistrationType(xaml::UIElement, DoubleTapped) eventRegistration =
        CreateSafeEventRegistration(xaml::UIElement, DoubleTapped);
};

    } } }
} } } }
