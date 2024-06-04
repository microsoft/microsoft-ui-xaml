// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EventTrigger.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(EventTrigger)
    {
    public:
        _Check_return_ HRESULT get_RoutedEventImpl(_Outptr_ xaml::IRoutedEvent** ppValue);
        _Check_return_ HRESULT put_RoutedEventImpl(_In_ xaml::IRoutedEvent* pValue);
    };
}
