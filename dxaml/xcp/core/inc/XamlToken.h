// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlTypeInfoProviderKind.h"
#include "XamlFactoryKind.h"

// A tokenization of part of a XAML file, storing enough data to map it
// to a concrete runtime type system.
struct XamlToken
{
    XamlToken()
        : m_Data(0)
    {}

    bool IsEmpty() const
    {
        return m_Data == 0;
    }

    XamlToken(const XamlTypeInfoProviderKind& inKind, const XUINT32& inHandle)
        : m_Data(inHandle | ((XUINT32)inKind << HANDLE_BIT_WIDTH))
    {
        ASSERT((inHandle & HANDLE_MASK) == inHandle);
    }

    void Reset()
    {
        m_Data = 0;
    }

    // The TypeInfoProvider and RuntimeKind have been merged together in Jupiter.
    void SetProviderKind(XamlTypeInfoProviderKind inProviderKind)
    {
        m_Data = (m_Data & HANDLE_MASK) | ((XUINT32)inProviderKind << HANDLE_BIT_WIDTH);
    }

    // The Provider of the XamlType identifies the type system that this token
    // speaks to. When classes need to query for deeper, type system specific
    // information about this type they will query for the provider kind and then
    // defer to that type system.
    XamlTypeInfoProviderKind GetProviderKind()  const
    {
        return (XamlTypeInfoProviderKind)(m_Data >> HANDLE_BIT_WIDTH);
    }

    // The Runtime of the XamlType identifies the runtime that this token speaks
    // to. When classes need to perform runtime actions against this TypeToken
    // (this creating an object, setting a property, etc) they will query for
    // the runtime kind and then defer to that runtime to perform the action.
    XamlFactoryKind GetRuntimeKind()  const
    {
        return (XamlFactoryKind)GetProviderKind();
    }

    // DEPRECATED. Used once today in ManagedTypeInfoProvider.cpp. Remove in the future
    // as time allows.
    HRESULT SetFromInt32(XUINT32 inData)
    {
        m_Data = inData;
        if ((XUINT32)GetProviderKind() >= (XUINT32)tpkNumberOfItems)
        {
            m_Data = 0;
            return E_UNEXPECTED;
        }

        return S_OK;
    }

protected:
    XUINT32 GetHandle() const
    {
        return m_Data & HANDLE_MASK;
    }
    void SetHandle(XUINT32 inHandle)
    {
        m_Data = (m_Data & ~HANDLE_MASK) | inHandle;
    }

private:
    static const XUINT32 HANDLE_MASK = 0x0FFFFFFF;
    static const int HANDLE_BIT_WIDTH = 28;
    XUINT32 m_Data;
};
