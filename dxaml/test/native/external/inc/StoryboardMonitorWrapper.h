// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

ref class StoryboardMonitorWrapper sealed
{
internal:

    void AttachStartedHandler(std::function<void(xaml_animation::Storyboard^, xaml::UIElement^)> startedCallback)
    {
        DetachStartedHandler();
        m_startedCallback = startedCallback;
        m_startedEventToken =
        test_infra::StoryboardMonitor::StoryboardStarted +=
            ref new test_infra::StoryboardEventHandler(this, &StoryboardMonitorWrapper::OnStoryboardStarted);
    }

    void DetachStartedHandler()
    {
        if (m_startedCallback)
        {
            test_infra::StoryboardMonitor::StoryboardStarted -= m_startedEventToken;
            m_startedCallback = nullptr;
            m_startedEventToken = {};
        }
    }

public:

    virtual ~StoryboardMonitorWrapper()
    {
        DetachStartedHandler();
    }

private:

    // Called on the UI thread.
    void OnStoryboardStarted(xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        try
        {
            m_startedCallback(storyboard, target);
        }
        CATCH_THROW_NORMALIZED();
    }

    std::function<void(xaml_animation::Storyboard^, xaml::UIElement^)> m_startedCallback;
    wf::EventRegistrationToken m_startedEventToken;
};

} } } } }
