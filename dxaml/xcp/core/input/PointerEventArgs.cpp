// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"

_Check_return_ HRESULT CPointerEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreatePointerRoutedEventArgs(this, ppPeer));
}
