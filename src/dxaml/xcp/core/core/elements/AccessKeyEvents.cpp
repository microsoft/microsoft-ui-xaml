// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "AccessKeyInvokedEventArgs.h"
#include "AccessKeyShownEventArgs.h"
#include "AccessKeyHiddenEventArgs.h"

_Check_return_ HRESULT CAccessKeyInvokedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    return DirectUI::OnFrameworkCreateAccessKeyInvokedEventArgs(this, ppPeer);
}

_Check_return_ HRESULT CAccessKeyDisplayRequestedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    return DirectUI::OnFrameworkCreateAccessKeyDisplayRequestedEventArgs(this, ppPeer);
}

_Check_return_ HRESULT CAccessKeyDisplayDismissedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    return DirectUI::OnFrameworkCreateAccessKeyDisplayDismissedEventArgs(this, ppPeer);
}

