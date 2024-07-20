// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyObjectCollection.g.h"
#include "VectorChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

DependencyObjectCollection::DependencyObjectCollection()
{
}

IFACEMETHODIMP
DependencyObjectCollection::SetAt(
    _In_ UINT index,
    _In_opt_ xaml::IDependencyObject* pItem)
{
    HRESULT hr = S_OK;
    UINT size = 0;
    CValue boxedValue;
    BoxerBuffer buffer;
    DependencyObject* pMOR = NULL;

    IFC(get_Size(&size));
    if (index >= size)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_DEPENDENCYOBJECTCOLLECTION_OUTOFRANGE));
    }

    IFC(CValueBoxer::BoxObjectValue(&boxedValue, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject), pItem, &buffer, &pMOR));

    IFC(CoreImports::DependencyObjectCollection_SetAt(
        static_cast<CDependencyObjectCollection*>(GetHandle()),
        index,
        &boxedValue));

Cleanup:
    ctl::release_interface(pMOR);
    RRETURN(hr);
}

IFACEMETHODIMP
DependencyObjectCollection::InsertAt(
    _In_ UINT index,
    _In_opt_ xaml::IDependencyObject* pItem)
{
    HRESULT hr = S_OK;

    UINT size = 0;

    IFC(get_Size(&size));
    if (index > size)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_DEPENDENCYOBJECTCOLLECTION_OUTOFRANGE));
    }

    IFC(DependencyObjectCollectionGenerated::InsertAt(index, pItem));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
DependencyObjectCollection::RemoveAt(
    _In_ UINT index)
{
    HRESULT hr = S_OK;
    UINT size = 0;

    IFC(get_Size(&size));
    if (index >= size)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_DEPENDENCYOBJECTCOLLECTION_OUTOFRANGE));
    }

    IFC(DependencyObjectCollectionGenerated::RemoveAt(index));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
DependencyObjectCollection::Append(
    _In_opt_ xaml::IDependencyObject* pItem)
{
    HRESULT hr = S_OK;

    IFC(DependencyObjectCollectionGenerated::Append(pItem));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
DependencyObjectCollection::Clear()
{
    RRETURN(DependencyObjectCollectionGenerated::Clear());
}

IFACEMETHODIMP DependencyObjectCollection::add_VectorChanged(
    _In_ wfc::VectorChangedEventHandler<xaml::DependencyObject*> *pHandler,
    _In_ EventRegistrationToken *token)
{
    HRESULT hr = S_OK;

    ARG_NOTNULL(pHandler, "handler");
    ARG_VALIDRETURNPOINTER(token);

    m_vectorChangedHandlers.AddHandler(pHandler, token);

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP DependencyObjectCollection::remove_VectorChanged(
    _In_ EventRegistrationToken token)
{
    RRETURN(m_vectorChangedHandlers.RemoveHandler(token));
}

_Check_return_ HRESULT
DependencyObjectCollection::RaiseVectorChanged(
    _In_ wfc::CollectionChange action,
    UINT index)
{
    HRESULT hr = S_OK;
    VectorChangedEventArgs *pArgs = NULL;

    IFC(ctl::ComObject<VectorChangedEventArgs>::CreateInstance(&pArgs));
    IFC(pArgs->put_CollectionChange(action));
    IFC(pArgs->put_Index(index));

    IFC(m_vectorChangedHandlers.Raise(this, pArgs));

Cleanup:
    ctl::release_interface(pArgs);

    RRETURN(hr);
}

void
DependencyObjectCollection::OnReferenceTrackerWalk(INT walkType)
{
    m_vectorChangedHandlers.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));

    DependencyObjectCollectionGenerated::OnReferenceTrackerWalk(walkType);
}

_Check_return_ HRESULT
DependencyObjectCollection::OnChildUpdated(_In_ DependencyObject *pChild)
{
    HRESULT hr = S_OK;

    IFC(DependencyObjectCollectionGenerated::OnChildUpdated(pChild));
    IFC(pChild->OnInheritanceContextChanged());

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DependencyObjectCollection::OnCollectionChanged(
    _In_ XUINT32 nCollectionChangeType,
    _In_ XUINT32 nIndex)
{
    RRETURN(RaiseVectorChanged((wfc::CollectionChange)(nCollectionChangeType), nIndex));
}
