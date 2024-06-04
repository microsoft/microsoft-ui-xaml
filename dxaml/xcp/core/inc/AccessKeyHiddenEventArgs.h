// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include "EventArgs.h"

class CAccessKeyDisplayDismissedEventArgs final : public CEventArgs
{
public:
    CAccessKeyDisplayDismissedEventArgs() = default;

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;
};

