// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Storyboard.g.h"

using namespace DirectUI;

_Check_return_ HRESULT StoryboardFactory::SetTargetImpl(_In_ xaml_animation::ITimeline* pTimeline, _In_ xaml::IDependencyObject* pTarget)
{
    HRESULT hr = S_OK;

    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(static_cast<Timeline*>(pTimeline)->GetHandle()), static_cast<DependencyObject*>(pTarget)->GetHandle()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::Storyboard::SeekImpl(_In_ wf::TimeSpan offset)
{
    HRESULT hr = S_OK;
    CValue box;

    IFC(CValueBoxer::BoxValue(&box, offset));

    IFC(CoreImports::CStoryboard_SeekPublic(static_cast<CStoryboard*>(GetHandle()), box));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::Storyboard::GetCurrentStateImpl(_Out_ xaml_animation::ClockState* returnValue)
{
    auto currentState = CoreImports::Timeline_GetClockState(checked_cast<CTimeline>(GetHandle()));
    *returnValue = static_cast<xaml_animation::ClockState>(currentState);
    return S_OK;
}

_Check_return_ HRESULT DirectUI::Storyboard::GetCurrentTimeImpl(_Out_ wf::TimeSpan* returnValue)
{
    wf::TimeSpan currentTimeSpan = {};
    double currentTime = CoreImports::Timeline_GetCurrentTime(checked_cast<CTimeline>(GetHandle()));

    currentTimeSpan.Duration = static_cast<INT64>(currentTime * TicksPerSecond);
    *returnValue = currentTimeSpan;

    return S_OK;
}

_Check_return_ HRESULT DirectUI::Storyboard::SeekAlignedToLastTickImpl(_In_ wf::TimeSpan offset)
{
    HRESULT hr = S_OK;
    CValue box;

    IFC(CValueBoxer::BoxValue(&box, offset));

    IFC(CoreImports::CStoryboard_SeekAlignedToLastTickPublic(static_cast<CStoryboard*>(GetHandle()), box));

Cleanup:
    RRETURN(hr);
}
