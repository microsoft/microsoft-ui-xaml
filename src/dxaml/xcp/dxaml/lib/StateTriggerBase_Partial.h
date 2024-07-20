// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "StateTriggerBase.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(StateTriggerBase)
    {
        public:
        _Check_return_ HRESULT SetActiveImpl(_In_ BOOLEAN bTriggerValue);
    };

}
