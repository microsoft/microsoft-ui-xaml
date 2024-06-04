// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCorePropertyChangedEventArgs final : public CEventArgs
{
public:
    CCorePropertyChangedEventArgs()
    {
        m_nPropertyIndex = 0;
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

public:
    XUINT32 m_nPropertyIndex;        // The index of the changed property
};

