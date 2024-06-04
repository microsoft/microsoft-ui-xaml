// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InputPointEventArgs.h"

class CCoreSerivces;
class CUIElement;

class CContextRequestedEventArgs final : public CInputPointEventArgs
{
public:
    CContextRequestedEventArgs(_In_ CCoreServices* pCore) : CInputPointEventArgs(pCore)
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT TryGetPosition(_In_opt_ CUIElement* pRelativeTo, _Out_ wf::Point* pPosition, _Out_ BOOLEAN* pReturnValue)
    {
        if (m_ptGlobal.x == -1 && m_ptGlobal.y == -1)
        {
            *pPosition = { 0, 0 };
            *pReturnValue = FALSE;
        }
        else
        {
            IFC_RETURN(GetPosition(pRelativeTo, pPosition));
            *pReturnValue = TRUE;
        }

        return S_OK;
    }
};
