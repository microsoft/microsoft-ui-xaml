// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThemeShadow.g.h"
#include "ThemeShadow.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ThemeShadowFactory::get_IsDropShadowModeImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = CThemeShadow::IsDropShadowMode();
    return S_OK;
}

