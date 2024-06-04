// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThemeShadow.g.h"
#include "ThemeShadow.h"



using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
ThemeShadow::get_MaskImpl(_Outptr_result_maybenull_ WUComp::ICompositionBrush** ppValue)
{
    CThemeShadow* coreThemeShadow = static_cast<CThemeShadow*>(GetHandle());
    IFC_RETURN(coreThemeShadow->GetMask(ppValue));

    return S_OK;
}

_Check_return_ HRESULT
ThemeShadow::put_MaskImpl(_In_opt_ WUComp::ICompositionBrush* pValue)
{
    CThemeShadow* coreThemeShadow = static_cast<CThemeShadow*>(GetHandle());
    coreThemeShadow->SetMask(pValue);

    return S_OK;
}
