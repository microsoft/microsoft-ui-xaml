// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarElementContainer.g.h"
#include "CommandBar.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace ::Windows::Internal;

AppBarElementContainer::AppBarElementContainer()
{
}

_Check_return_ HRESULT AppBarElementContainer::get_IsInOverflowImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(CommandBar::IsCommandBarElementInOverflow(this, pValue));
    return S_OK;
}

IFACEMETHODIMP AppBarElementContainer::OnApplyTemplate()
{
    return S_OK;
}