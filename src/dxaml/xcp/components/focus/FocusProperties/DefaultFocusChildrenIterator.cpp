// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <ComObject.h>
#include <DependencyObjectAbstractionHelpers.h>
#include <UIElement.h>
#include <DOCollection.h>
#include <DXamlServices.h>

#include "DefaultFocusChildrenIterator.h"
#include "FocusProperties.h"

using namespace DirectUI;
using namespace FocusProperties;

/*static*/ _Check_return_ HRESULT
DefaultFocusChildrenIterator::CreateInstance(
    _In_ xaml::IDependencyObject* parent,
    _Outptr_ wfc::IIterator<xaml::DependencyObject*>** instance)
{
    ctl::ComPtr<DefaultFocusChildrenIterator> iterator;
    IFC_RETURN(ctl::ComObject<DefaultFocusChildrenIterator>::CreateInstance(iterator.GetAddressOf()));
    iterator->SetPtrValue(iterator->m_parent, parent);

    *instance = iterator.Detach();
    return S_OK;
}

_Check_return_ IFACEMETHODIMP
DefaultFocusChildrenIterator::get_Current(_Outptr_ xaml::IDependencyObject** current)
{
    *current = nullptr;
    IFC_RETURN(CheckThread());
    if (m_index < GetCount())
    {
        IFC_RETURN(GetAt(m_index, current));
    }
    else
    {
        IFC_RETURN(E_BOUNDS);
    }
    return S_OK;
}

_Check_return_ IFACEMETHODIMP
DefaultFocusChildrenIterator::get_HasCurrent(_Out_ BOOLEAN *hasCurrent)
{
    *hasCurrent = false;
    IFC_RETURN(CheckThread());
    *hasCurrent = m_index < GetCount();
    return S_OK;
}

_Check_return_ IFACEMETHODIMP
DefaultFocusChildrenIterator::MoveNext(_Out_ BOOLEAN *hasCurrent)
{
    *hasCurrent = false;
    IFC_RETURN(CheckThread());
    ++m_index;
    const unsigned count = GetCount();
    if (m_index < count)
    {
        *hasCurrent = true;
    }
    else
    {
        m_index = count;
    }
    return S_OK;
}

_Check_return_ HRESULT
DefaultFocusChildrenIterator::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** object)
{
    if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterator<xaml::DependencyObject*>)))
    {
        *object = static_cast<wfc::IIterator<xaml::DependencyObject*> *>(this);
    }
    else
    {
        return ctl::WeakReferenceSource::QueryInterfaceImpl(iid, object);
    }

    AddRefOuter();
    return S_OK;
}

unsigned DefaultFocusChildrenIterator::GetCount()
{
    auto parent = DependencyObjectAbstractionHelpers::GetHandle(DependencyObjectAbstractionHelpers::IDOtoDO(m_parent.Get()));
    auto children = FocusProperties::GetFocusChildren<CDOCollection>(parent);
    return (children && !children->IsLeaving()) ? children->GetCount() : 0;
}

_Check_return_ HRESULT
DefaultFocusChildrenIterator::GetAt(_In_ unsigned index, _Outptr_ xaml::IDependencyObject** value)
{
    auto parent = DependencyObjectAbstractionHelpers::GetHandle(DependencyObjectAbstractionHelpers::IDOtoDO(m_parent.Get()));
    auto children = FocusProperties::GetFocusChildren<CDOCollection>(parent);

    DependencyObject* peer = nullptr;
    IFC_RETURN(DXamlServices::GetPeer(children->GetItemImpl(index), &peer));
    *value = DependencyObjectAbstractionHelpers::DOtoIDO(peer);

    return S_OK;
}