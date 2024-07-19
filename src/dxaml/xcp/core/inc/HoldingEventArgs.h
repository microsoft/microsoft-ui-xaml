// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CHoldingEventArgs final : public CInputPointEventArgs
{
public:
    CHoldingEventArgs(_In_ CCoreServices* pCore) : CInputPointEventArgs(pCore)
    {
        m_holdingState = DirectUI::HoldingState::Started;
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_HoldingState(_Out_ DirectUI::HoldingState* pValue)
    {
        *pValue = m_holdingState;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_HoldingState(_In_ DirectUI::HoldingState value)
    {
        m_holdingState = value;
        RRETURN(S_OK);
    }

    DirectUI::HoldingState m_holdingState;
};
