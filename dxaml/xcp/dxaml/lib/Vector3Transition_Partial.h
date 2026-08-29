// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Vector3Transition.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(Vector3Transition)
    {
        protected:
            Vector3Transition() = default;
            ~Vector3Transition() override = default;

            HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
    };
}
