// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EasingFunctionBase.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(EasingFunctionBase)
    {

    public:

        _Check_return_ HRESULT EaseImpl(_In_ DOUBLE normalizedTime, _Out_ DOUBLE* returnValue);

    };
}
