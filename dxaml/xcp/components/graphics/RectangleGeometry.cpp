// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RectangleGeometry.h>

_Check_return_ HRESULT CRectangleGeometry::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;
    CRectangleGeometry *_this;

    _this = new CRectangleGeometry(pCreate->m_pCore);

    *ppObject = (CDependencyObject *) _this;
    RRETURN(hr);//RRETURN_REMOVAL
}
