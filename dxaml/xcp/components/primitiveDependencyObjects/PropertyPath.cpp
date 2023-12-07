// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CPropertyPath.h>

_Check_return_ HRESULT CPropertyPath::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    CPropertyPath* _this = nullptr;

    *ppObject = nullptr;

    _this = new CPropertyPath(pCreate->m_pCore);

    if (pCreate->m_value.GetType() == valueString)
    {
        _this->m_strPath = pCreate->m_value.AsString();
    }

    // Return the object to the caller
    *ppObject = static_cast<CDependencyObject *>(_this);
    
    return S_OK;
}