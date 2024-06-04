// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

HRESULT XamlQualifiedName::get_ScopedName(_Out_ xstring_ptr* pstrOutPrefix)
{
    HRESULT hr = S_OK;

    if (m_ssScopedName.IsNull())
    {
        // Cache it on first access
        if (m_ssPrefix.GetCount() == 0)
        {
            // Name
            m_ssScopedName = m_ssName;
        }
        else
        {
            // Prefix.Name
            XStringBuilder scopedNameBuilder;

            IFC(scopedNameBuilder.Initialize(m_ssPrefix.GetCount() + 1 + m_ssName.GetCount()));
            IFC(scopedNameBuilder.Append(m_ssPrefix));
            IFC(scopedNameBuilder.AppendChar(L'.'));
            IFC(scopedNameBuilder.Append(m_ssName));

            IFC(scopedNameBuilder.DetachString(&m_ssScopedName));
        }
    }

    *pstrOutPrefix = m_ssScopedName;

Cleanup:
    if (FAILED(hr))
    {
        m_ssScopedName.Reset();
    }

    RRETURN(hr);
}
