// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   CSharedName::Create - from length counted string
//
//  Synopsis:
//      Allocates memory for the string and copies
//      it. Guarantees zero termination.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CSharedName::Create(
    _In_                       XUINT32       cString, 
    _In_reads_(cString) const WCHAR        *pString,
    _Outptr_                CSharedName **ppName
)
{
    return CSharedName::Create(xephemeral_string_ptr(pString, cString), ppName);
}


//------------------------------------------------------------------------
//
//  Method:   CSharedName::Create - from zero terminated string
//
//  Synopsis:
//      Allocates memory for the string and copies
//      it. Guarantees zero termination.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CSharedName::Create(
    _In_ const xstring_ptr_view& strName,
    _Outptr_  CSharedName **ppName
    )
{
    auto result = make_xref<CSharedName>();
    IFC_RETURN(strName.Promote(&result->m_strName));
    *ppName = result.detach();
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CSharedName::AddRef
//
//  Synopsis:
//      Raises the reference count on the object.
//
//------------------------------------------------------------------------

XUINT32 CSharedName::AddRef()
{
    ++m_cRef;
    if (!m_cRef)
    {
        // we have hit an overflow...exit the process
        XAML_FAIL_FAST();
    }
    return m_cRef;
}

//------------------------------------------------------------------------
//
//  Method:   CSharedName::Release
//
//  Synopsis:
//      Lowers the reference count on the object.  Will delete it when there
// are no remaining references.
//
//------------------------------------------------------------------------

XUINT32 CSharedName::Release()
{
    XUINT32 cRef = --m_cRef;

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

