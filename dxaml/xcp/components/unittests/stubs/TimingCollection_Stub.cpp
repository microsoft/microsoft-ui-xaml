// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xstring_ptr.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif


#include "TimingCollection.h"

void CTimingCollection::SetTimingOwner( _In_ CTimeline* pTimingOwner )
{
}

_Check_return_ HRESULT CTimingCollection::CheckCanBeModified()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimingCollection::Clear()
{
    return CDOCollection::Clear();
}

_Check_return_ HRESULT CTimingCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    return CDOCollection::Append(pObject, pnIndex);
}

_Check_return_ HRESULT CTimingCollection::Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject)
{
    return E_NOTIMPL;
}

_Check_return_ void* CTimingCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    return CDOCollection::RemoveAt(nIndex);
}

