// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BitmapImage.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(BitmapImage)
    {
        friend class BitmapImageFactory;

    public:
        _Check_return_ HRESULT PlayImpl();
        _Check_return_ HRESULT StopImpl();

        IFACEMETHOD(SetSource)(_In_ wsts::IRandomAccessStream* pStreamSource) override;
    };
}
