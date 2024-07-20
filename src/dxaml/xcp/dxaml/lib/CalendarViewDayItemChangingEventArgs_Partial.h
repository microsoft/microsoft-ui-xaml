// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarViewDayItemChangingEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(CalendarViewDayItemChangingEventArgs)
    {
    protected:
        CalendarViewDayItemChangingEventArgs() = default;

    public:
        _Check_return_ HRESULT RegisterUpdateCallbackImpl(_In_ wf::ITypedEventHandler<xaml_controls::CalendarView*, xaml_controls::CalendarViewDayItemChangingEventArgs*>* pCallback);
        _Check_return_ HRESULT RegisterUpdateCallbackWithPhaseImpl(_In_ UINT callbackPhase, _In_ wf::ITypedEventHandler<xaml_controls::CalendarView*, xaml_controls::CalendarViewDayItemChangingEventArgs*>* pCallback);
        _Check_return_ HRESULT ResetLifetimeImpl();
    };
}
