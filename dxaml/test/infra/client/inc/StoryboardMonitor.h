// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {
    
    // Monitors storyboard events and report them to the test infrastructure.
    // Currently, it monitors only the storyboard 'started' event.
    // StoryboardMonitor is not thread safe.
    class StoryboardMonitorStatics
        : public wrl::AgileActivationFactory<test_infra::IStoryboardMonitorStatics>
    {
        InspectableClassStatic(RuntimeClass_Private_Infrastructure_StoryboardMonitor, TrustLevel::BaseTrust);

    public:

        IFACEMETHOD(add_StoryboardStarted)(test_infra::IStoryboardEventHandler *pHandler, EventRegistrationToken *pToken) override;
        IFACEMETHOD(remove_StoryboardStarted)(EventRegistrationToken token) override;

        static HRESULT OnStoryboardStarted(xaml_animation::IStoryboard* pStoryboard, xaml::IUIElement* pTarget);

    private:
        
        static wrl::EventSource<test_infra::IStoryboardEventHandler> s_storyboardStartedEventSource;
        static unsigned s_totalActiveHandlers;
    };

} }
