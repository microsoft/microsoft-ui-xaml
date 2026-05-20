// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StoryboardMonitor.h"
#include "WindowHelper.h"

#include <IXamlTestHooks-win.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;

namespace Private { namespace Infrastructure {

wrl::EventSource<test_infra::IStoryboardEventHandler> StoryboardMonitorStatics::s_storyboardStartedEventSource;
unsigned StoryboardMonitorStatics::s_totalActiveHandlers = 0;

IFACEMETHODIMP
StoryboardMonitorStatics::add_StoryboardStarted(
    test_infra::IStoryboardEventHandler *pHandler,
    EventRegistrationToken *pToken) /* override */
{
    try
    {
        THROW_IF_FAILED(s_storyboardStartedEventSource.Add(pHandler, pToken));
        if (s_totalActiveHandlers == 0)
        {
            THROW_IF_FAILED(WindowHelper::GetTestHooks()->SetStoryboardStartedCallback(&OnStoryboardStarted));
        }
        ++s_totalActiveHandlers;
    }
    CATCH_RETURN();
    return S_OK;
}

IFACEMETHODIMP
StoryboardMonitorStatics::remove_StoryboardStarted(
    EventRegistrationToken token) /* override */
{
    try
    {
        THROW_IF_FAILED(s_storyboardStartedEventSource.Remove(token));
        --s_totalActiveHandlers;
        if (s_totalActiveHandlers == 0)
        {
            THROW_IF_FAILED(WindowHelper::GetTestHooks()->SetStoryboardStartedCallback(nullptr));
        }
    }
    CATCH_RETURN();
    return S_OK;
}

/*static*/
HRESULT StoryboardMonitorStatics::OnStoryboardStarted(
    xaml_animation::IStoryboard* pStoryboard,
    xaml::IUIElement* pTarget)
{
    return s_storyboardStartedEventSource.InvokeAll(pStoryboard, pTarget);
}

} }
