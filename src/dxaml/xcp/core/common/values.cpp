// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Default cleanup callback for object-based value stores.
//
//------------------------------------------------------------------------
void __stdcall
DefaultObjectBaseValueStoreCleanup(
    _In_ XHANDLE hValue,
    _In_opt_ XHANDLE
    )
{
    if (hValue)
    {
        IObject* pObject = reinterpret_cast<IObject*>(hValue);

        pObject->Release();
    }
}

//------------------------------------------------------------------------
//
//  Method:   ctor
//
//  Synopsis:
//      Constructor for CValueStore object
//
//------------------------------------------------------------------------

CValueStore::CValueStore(bool bObjectBase, CDependencyObject* pOwner)
    : m_cRef(1)
    , m_bObjectBase(bObjectBase)
    , m_pCleanupCallback(bObjectBase ? DefaultObjectBaseValueStoreCleanup : nullptr)
    , m_pOwner(pOwner)
{
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for CValueStore object
//
//------------------------------------------------------------------------

CValueStore::~CValueStore()
{
    ASSERT(m_pCleanupCallback || !m_bObjectBase);

    if (m_pCleanupCallback)
    {
        for (auto it = m_map.begin(); it != m_map.end(); ++it)
        {
            m_pCleanupCallback(it->second, m_pOwner);
        }
    }
}

//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Lower the reference count on the object.  When it reaches zero the
//  object will be deleted.
//
//------------------------------------------------------------------------

XUINT32
CValueStore::Release()
{
    XUINT32 cRef = --m_cRef;

    if (!cRef)
    {
        delete this;
    }

    return cRef;
}

//------------------------------------------------------------------------
//
//  Method:   PutValue
//
//  Synopsis:
//      Stores a name/value pair.  If the name is not yet known the function
//  will attempt to add it to the known list of names if the name is known it
//  will update the value associated with the name.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CValueStore::PutValue(
    _In_ const xstring_ptr_view& strName,
    _In_ XHANDLE hValue
    )
{
    xstring_ptr xstrptrName;
    IFC_RETURN(strName.Promote(&xstrptrName));

    if (hValue)
    {
        m_map[xstrptrName] = hValue ;

        if (m_bObjectBase)
        {
            reinterpret_cast<IObject*>(hValue)->AddRef();
        }
    }
    else
    {
        m_map.erase(xstrptrName);
    }

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method:   GetValue
//
//  Synopsis:
//      Recalls the value associated with the name.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CValueStore::GetValue(
    _In_ const xstring_ptr_view& strName,
    _Out_ XHANDLE *phValue
    )
{
    auto itValue = m_map.find(strName);
    if (itValue == m_map.end())
    {
        *phValue = nullptr;
        return E_UNEXPECTED;
    }
    else
    {
        *phValue = itValue->second;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Traverse
//
//  Synopsis:
//      Walks the trie making a callback for each key-value pair found.
//    This walk should be in alphabetical order.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CValueStore::Traverse(_In_ ValueStoreTraversalCallback pCallback, _In_ XHANDLE pExtraData)
{
    IFCPTR_RETURN(pCallback);

    for (auto it = m_map.begin(); it != m_map.end(); ++it)
    {
        pCallback(it->first, it->second, pExtraData);
    }

    return S_OK;
}