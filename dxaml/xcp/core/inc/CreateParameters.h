// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CValue.h"
#include "XcpDebug.h"

class CCoreServices;
class XamlServiceProviderContext;

// These are the creation parameters passed to the create function for all the
// classes derived from CObject.
class CREATEPARAMETERS
{
public:
    CREATEPARAMETERS() = delete;

    CREATEPARAMETERS(_In_ CCoreServices *pCore)
    {
        XCP_WEAK(&m_pCore);
        m_pCore = pCore;
    }
    CREATEPARAMETERS(_In_ CCoreServices *pCore, _In_ const CValue& value)
    {
        XCP_WEAK(&m_pCore);
        m_pCore = pCore;
        m_value.WrapValue(value);
    }
    CREATEPARAMETERS(_In_ CCoreServices *pCore, _In_ const xstring_ptr& string)
    {
        XCP_WEAK(&m_pCore);
        m_pCore = pCore;
        m_value.SetString(string);
    }
    CValue m_value;
    CCoreServices *m_pCore;
    std::shared_ptr<XamlServiceProviderContext> m_spServiceProviderContext;
};
