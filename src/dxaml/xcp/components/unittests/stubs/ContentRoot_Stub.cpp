// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContentRoot.h"

CXamlIslandRoot* CContentRoot::GetXamlIslandRootNoRef() const
{
    return nullptr;
}

_Check_return_ HRESULT CContentRoot::SetContentIsland(_In_ ixp::IContentIsland* compositionContent)
{
    return S_OK;
}

XUINT32 CContentRoot::Release()
{
    return 1;
}
