// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlLightCollection.h>
#include <XamlLight.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <ICollectionChangeCallback.h>
#include <UIElement.h>

CXamlLightCollection::CXamlLightCollection(_In_ CCoreServices* core)
    : CDOCollection(core)
{}

CXamlLightCollection::~CXamlLightCollection()
{
    VERIFYHR(Destroy());
}

_Check_return_ bool CXamlLightCollection::NeedsOwnerInfo()
{
    return true;
}

KnownTypeIndex CXamlLightCollection::GetTypeIndex() const
{
    return DependencyObjectTraits<CXamlLightCollection>::Index;
}

void CXamlLightCollection::SetChangeCallback(_In_ const std::weak_ptr<ICollectionChangeCallback>& callback)
{
    m_wrChangeCallback = callback;
}

_Check_return_ HRESULT CXamlLightCollection::Create(
    _Outptr_ CDependencyObject** object,
    _In_ CREATEPARAMETERS* create)
{
    *object = new CXamlLightCollection(create->m_pCore);
    return S_OK;
}


//
//  CLightCollection::RemoveAt
//
//  Clear Logical child before an item is removed from the collection
//
_Check_return_ void* CXamlLightCollection::RemoveAt(_In_ XUINT32 index)
{
    CDependencyObject* dependencyObject = GetItemDOWithAddRef(index);
    // We assume that we got correct index from managed side.
    // If somebody going to decide to use ItemCollection from native
    // we will assert in case of wrong index.
    ASSERT(dependencyObject != nullptr);

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFCFAILFAST(callback->ElementRemoved(index));
        }
    }

    ReleaseInterface(dependencyObject);
    return CDOCollection::RemoveAt(index);
}

//  CLightCollection::Append
//
//  Before calling to base to add the item, validate that
//  it's OK to be using Items (i.e. that ItemsSource isn't being
//  used right now).
//
_Check_return_ HRESULT CXamlLightCollection::Append(_In_ CDependencyObject* object, _Out_opt_ XUINT32* index)
{
    if (object == nullptr)
    {
        return E_INVALIDARG;
    }

    IFC_RETURN(CDOCollection::Append(object, index));

    return S_OK;
}

//
//  CLightCollection::Insert
//
//  Before calling to base to insert the item, validate that
//  it's OK to be using Items (i.e. that ItemsSource isn't being
//  used right now).
//
_Check_return_ HRESULT CXamlLightCollection::Insert(_In_ XUINT32 index, _In_ CDependencyObject* object)
{
    if (object == nullptr)
    {
        return E_INVALIDARG;
    }

    IFC_RETURN(CDOCollection::Insert(index, object));

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFCFAILFAST(callback->ElementInserted(index));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CXamlLightCollection::Clear()
{
    IFC_RETURN(CDOCollection::Clear());

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFCFAILFAST(callback->CollectionCleared());
        }
    }

    // Explicitly dirty the UIElement again. The Clear() call above unparented all the children, which called the dirty flag
    // propagation method, but that was before the collection was actually cleared. When the UIElement checked for elements
    // in its light collection, it would have seen a non-zero number of elements. Now that the items are actually gone, dirty
    // the UIElement again so it can make the correct check.
    CDependencyObject* parent = GetParentInternal(false);
    ASSERT(parent != nullptr && parent->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement::NWSetLightCollectionDirty(parent, DirtyFlags::Render);

    return S_OK;
}
