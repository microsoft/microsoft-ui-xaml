// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyFrame.h"
#include "KeyTime.h"

_Check_return_ HRESULT CKeyFrame::InitInstance()
{
    auto core = GetContext();
    m_keyTime = KeyTimeVOHelper::Create(core);
    return S_OK;
}
