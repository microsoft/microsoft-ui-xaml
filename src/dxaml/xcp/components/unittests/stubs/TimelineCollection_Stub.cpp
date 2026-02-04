// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xstring_ptr.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif


#include "TimelineCollection.h"

KnownTypeIndex CTimelineCollection::GetTypeIndex() const
{
    return KnownTypeIndex::TimelineCollection;
}

CDependencyObject* CTimelineCollection::GetStandardNameScopeParent()
{
    return nullptr;
}

_Check_return_ HRESULT CTimelineCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    return CDOCollection::Append(pObject, pnIndex);
}

_Check_return_ HRESULT CTimelineCollection::CycleCheck(_In_ CDependencyObject *pObject)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimelineCollection::OnAddToCollection(CDependencyObject *pDO)
{
    return E_NOTIMPL;
}
