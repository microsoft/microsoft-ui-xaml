// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "UIElementWeakCollection.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(UIElementWeakCollection)
    {
        protected:
            UIElementWeakCollection() = default;
            ~UIElementWeakCollection() override = default;

            HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
    };
}
