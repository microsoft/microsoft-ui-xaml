// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StaticStore.h"

namespace DesktopUtility {
    bool IsOnDesktop() { return true; }
};

namespace DirectUI
{

_Check_return_ HRESULT StaticStore::GetUriFactory(_COM_Outptr_ wf::IUriRuntimeClassFactory** result)
{
    *result = nullptr;
    return S_OK;
}

}