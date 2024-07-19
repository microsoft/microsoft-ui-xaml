// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Shape.g.h"
#include "Shape.h"

using namespace DirectUI;

_Check_return_ HRESULT Shape::GetAlphaMaskImpl(
    _Outptr_ WUComp::ICompositionBrush** ppResult)
{
    CShape* pShape = static_cast<CShape*>(GetHandle());
    IFC_RETURN(pShape->GetAlphaMask(ppResult));
    return S_OK;
}