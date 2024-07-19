// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CString.h>

_Check_return_ HRESULT
CString::CreateFromXStringPtr(
    _In_ CCoreServices *pCore,
    _Inout_ xstring_ptr&& strString,
    _Outptr_ CDependencyObject **ppObject)
{
    HRESULT hr = S_OK;

    *ppObject = new CString(pCore, std::forward<xstring_ptr>(strString));

    RRETURN(hr);//RRETURN_REMOVAL
} 

_Check_return_ HRESULT
CString::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT hr = S_OK;
    CString *pStringObject = NULL;

    // This method only expects string or the default
    ASSERT(pCreate->m_value.GetType() == valueString ||
           pCreate->m_value.GetType() == valueAny);

    // Create the string object
    pStringObject = new CString(pCreate->m_pCore, pCreate->m_value.AsString());

    *ppObject = pStringObject;

    RRETURN(hr);//RRETURN_REMOVAL
}

KnownTypeIndex CString::GetTypeIndex() const
{
    return DependencyObjectTraits<CString>::Index;
}

// Creates an instance of a string that is not ref counted.
_Check_return_ HRESULT CString::GetRawStringClone(_Outptr_result_maybenull_z_ WCHAR** ppString)
{
     HRESULT hr = S_OK;
     WCHAR *pString = NULL;

    // Clone the string
    if (!m_strString.IsNull())
    {
        if (ppString && m_strString.GetCount())
        {
            pString = new WCHAR[m_strString.GetCount() + 1];

            memcpy(pString, m_strString.GetBuffer(), sizeof(WCHAR)*m_strString.GetCount());
            pString[m_strString.GetCount()] = '\0';
        }
    }
    *ppString = pString;

    RRETURN(hr);//RRETURN_REMOVAL
}
