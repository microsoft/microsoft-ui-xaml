// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RoutedEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(RoutedEventArgs)
    {
    public:
        _Check_return_ HRESULT IsHandled(_Out_ bool* pHandled);
        _Check_return_ HRESULT SetHandled(_In_ bool bHandled);

        _Check_return_ HRESULT get_OriginalSourceImpl(_Outptr_ IInspectable** ppValue);
        _Check_return_ HRESULT put_OriginalSourceImpl(_In_ IInspectable* pValue);

    protected:
        CEventArgs* CreateCorePeer() override;
    };
}
