// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xstring_ptr.h"


#include "UIElement.h"
#include "UIElementCollection.h"

CUIElementCollection::~CUIElementCollection() {}
_Check_return_ HRESULT CUIElementCollection::SetOwner(_In_opt_ CDependencyObject *pOwner, _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag) { return E_NOTIMPL; }
_Check_return_ HRESULT CUIElementCollection::OnAddToCollection(_In_ CDependencyObject *pDO) { return E_NOTIMPL; }
_Check_return_ HRESULT CUIElementCollection::OnRemoveFromCollection(_Inout_opt_ CDependencyObject *pDO, _In_ XINT32 iPreviousIndex) { return E_NOTIMPL; }

_Check_return_ HRESULT CUIElementCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    IFCFAILFAST(CDOCollection::Append(pObject, pnIndex));
    return S_OK;
}

_Check_return_ HRESULT CUIElementCollection::Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject) { return E_NOTIMPL; }
_Check_return_ CDependencyObject* CUIElementCollection::Remove(_In_ CDependencyObject *pObject) { return nullptr; }

_Check_return_ void* CUIElementCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    return CDOCollection::RemoveAt(nIndex);
}

_Check_return_ HRESULT CUIElementCollection::Clear()
{
    for (auto idx = GetCount(); idx--;)
    {
        CUIElement* child = static_cast<CUIElement*>(RemoveAt(idx));
        ReleaseInterface(child);
    }
    return S_OK;
}

_Check_return_ HRESULT CUIElementCollection::MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition) { return E_NOTIMPL; }
_Check_return_ CDependencyObject* CUIElementCollection::RemoveAtImpl(_In_ XUINT32 nIndex) { return nullptr; }

_Check_return_ HRESULT CUIElementCollection::ValidateItem(_In_ CDependencyObject *pObject)
{
    return S_OK;
}

void CUIElementCollection::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) { }
_Check_return_ HRESULT CUIElementCollection::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner,_In_ LeaveParams params) { return E_NOTIMPL; }
void CUIElementCollection::OnRemovingDependencyObject(_In_ CDependencyObject* pDO) { }

_Check_return_ HRESULT CUIElementCollection::RemoveUnloadedElement(_In_ CUIElement* pTarget, UINT unloadContext, _Out_ bool* pfExecutedUnload)
{
    return E_NOTIMPL;
}

bool CUIElementCollection::IsUnloadingElement(_In_ CUIElement* pTarget)
{
    return false;
}

