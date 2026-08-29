// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct PropertyChangedParams
{
    const CDependencyProperty* m_pDP;
    const CValue* m_pOldValue;
    const CValue* m_pNewValue;
    IInspectable* m_pOldValueOuterNoRef;
    IInspectable* m_pNewValueOuterNoRef;

    PropertyChangedParams(_In_ const CDependencyProperty* pDP, _In_ const CValue& oldValue, _In_ const CValue& newValue, _In_opt_ IInspectable* pOldValueOuterNoRef = nullptr, _In_opt_ IInspectable* pNewValueOuterNoRef = nullptr)
        : m_pDP(pDP), m_pOldValue(&oldValue), m_pNewValue(&newValue), m_pOldValueOuterNoRef(pOldValueOuterNoRef), m_pNewValueOuterNoRef(pNewValueOuterNoRef)
    {
        ASSERT(pDP != nullptr);
    }
};
