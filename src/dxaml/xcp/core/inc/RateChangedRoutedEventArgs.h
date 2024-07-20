// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CRateChangedRoutedEventArgs final : public CRoutedEventArgs
{
public:
    CRateChangedRoutedEventArgs()
    {
        m_rRate = 0.0f;
    }

    // Destructor
    ~CRateChangedRoutedEventArgs() override
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_result_nullonfailure_ IInspectable** ppPeer) override;

    XFLOAT m_rRate;
};
