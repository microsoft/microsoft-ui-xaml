// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "KeyRoutedEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(KeyRoutedEventArgs)
    {
    public:
        _Check_return_ HRESULT get_KeyStatusImpl(_Out_ wuc::CorePhysicalKeyStatus* pValue);
    };
}
