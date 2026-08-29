// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CTextChangedEventArgs final : public CRoutedEventArgs
{
public:
    CTextChangedEventArgs()
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;
};
