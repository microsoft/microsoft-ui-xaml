// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CString.h"

struct XTABLE;

//------------------------------------------------------------------------
//
//  Class:  CFontWeight
//
//  Synopsis:
//      Created by XML parser to hold a font weight value
//
//------------------------------------------------------------------------

class CFontWeight : public CEnumerated
{
public:
// Creation method

    static
    _Check_return_ HRESULT Create(
            _Outptr_ CDependencyObject **ppObject,
            _In_ CREATEPARAMETERS *pCreate);
};

//------------------------------------------------------------------------
//
//  Class:  CUri
//
//  Synopsis:
//      Uri type converter.
//
//------------------------------------------------------------------------

class CUri
{
public:
    // Creation method

    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);
};
