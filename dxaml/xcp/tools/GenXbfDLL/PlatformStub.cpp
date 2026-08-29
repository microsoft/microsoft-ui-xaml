// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <cstddef>
#include <TemplateContent.h>

using namespace DirectUI;

_Check_return_ HRESULT CWinUriFactory::Create(_In_ XUINT32 cString, _In_reads_(cString) const WCHAR *pString, _Out_ IPALUri **ppUri)
{
    return E_NOTIMPL;
}

long __stdcall CString::CreateFromXStringPtr(class CCoreServices *,_Inout_ xstring_ptr&& strString,class CDependencyObject * *)
{
    ASSERT(FALSE);
    return E_FAIL;
}

long __stdcall CKeyTime::Create(class CDependencyObject * *,class CREATEPARAMETERS *)
{
    ASSERT(FALSE);
    return E_FAIL;
}

_Check_return_ std::size_t CTemplateContent::GetOrCreateNameIndex(_In_ const xstring_ptr_view& )
{
    ASSERT(FALSE);
    return static_cast<size_t>(-1);
}

_Check_return_ std::size_t CTemplateContent::TryGetNameIndex(_In_ const xstring_ptr_view&) const
{
    ASSERT(FALSE);
    return static_cast<size_t>(-1);
}