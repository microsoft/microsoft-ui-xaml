// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include "EventArgs.h"

class CKeyboardAcceleratorInvokedEventArgs final : public CEventArgs
{
public:
    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::KeyboardAcceleratorInvokedEventArgs;
    }
};

