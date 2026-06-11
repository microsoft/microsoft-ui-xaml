// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <ComObject.h>
#include "DefaultFocusChildrenIterable.h"
#include "DefaultFocusChildrenIterator.h"

using namespace FocusProperties;

/*static*/ _Check_return_ HRESULT
DefaultFocusChildrenIterable::CreateInstance(_In_ xaml::IDependencyObject* parent, _Outptr_ wfc::IIterable<xaml::DependencyObject*>** instance)
{
    ctl::ComPtr<DefaultFocusChildrenIterable> iterable;
    IFC_RETURN(ctl::ComObject<DefaultFocusChildrenIterable>::CreateInstance(iterable.GetAddressOf()));
    iterable->SetPtrValue(iterable->m_parent, parent);

    *instance = iterable.Detach();
    return S_OK;
}

_Check_return_ IFACEMETHODIMP
DefaultFocusChildrenIterable::First(_Outptr_ wfc::IIterator<xaml::DependencyObject*> **first)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(DefaultFocusChildrenIterator::CreateInstance(m_parent.Get(), first));
    return S_OK;
}

_Check_return_ HRESULT
DefaultFocusChildrenIterable::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** object)
{
    if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<xaml::DependencyObject*>)))
    {
        *object = static_cast<wfc::IIterable<xaml::DependencyObject*> *>(this);
    }
    else
    {
        return ctl::WeakReferenceSource::QueryInterfaceImpl(iid, object);
    }

    AddRefOuter();
    return S_OK;
}
