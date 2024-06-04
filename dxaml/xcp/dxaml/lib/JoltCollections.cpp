// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "JoltCollections.h"

using namespace DirectUI;

#pragma region View And Non-DO-Backed Collections
IFACEMETHODIMP 
View<IInspectable*>::IndexOf(
    _In_opt_ IInspectable* value, 
    _Out_ UINT *index, 
    _Out_ BOOLEAN *found)
{
    HRESULT hr = S_OK;
    std::list<IInspectable*>::iterator it;
    UINT nPosition = 0;

    IFC(CheckThread());
    for (it = m_list.begin(); it != m_list.end(); ++nPosition, ++it)
    {
        bool areEqual = false;
        IFC(PropertyValue::AreEqual(value, *it, &areEqual));
        if (areEqual)
        {
            *index = nPosition;
            *found = TRUE;
            goto Cleanup;
        }
    }

    *index = 0;
    *found = FALSE;
    IFC(S_FALSE);

Cleanup:
    RRETURN(hr);
}

#pragma endregion

#pragma region DO-backed Value Collection Specializations
IFACEMETHODIMP
PresentationFrameworkCollection<FLOAT>::Append(_In_ FLOAT item)
{
    HRESULT hr = S_OK;
    CValue boxedValue;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, item));

    IFC(CoreImports::Collection_Add(
        static_cast<CCollection*>(GetHandle()),
        &boxedValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP 
PresentationFrameworkCollection<FLOAT>::GetAt(
    _In_opt_ UINT index, 
    _Out_ FLOAT *item)
{
    HRESULT hr = S_OK;
    CValue value;
    XINT32 nIndex = static_cast<XINT32>(index);
    
    IFC(CheckThread());
    IFC(CoreImports::Collection_GetItem(static_cast<CCollection*>(GetHandle()), nIndex, &value));
    IFC(CValueBoxer::UnboxValue(&value, item));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<FLOAT>::IndexOf(_In_ FLOAT value, _Out_ UINT *index, _Out_ BOOLEAN *found)
{
    HRESULT hr = S_FALSE;
    CValue boxedValue;
    XINT32 coreIndex = -1;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, value));

    if (SUCCEEDED(CoreImports::Collection_IndexOf(
        static_cast<CCollection*>(GetHandle()),
        &boxedValue,
        &coreIndex)))
    {
        *index = static_cast<UINT>(coreIndex);
    }
    else
    {
        IFC(S_FALSE);
    }

    *found = coreIndex != -1;

Cleanup:      
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<FLOAT>::InsertAt(_In_ UINT index, _In_ FLOAT item)
{
    HRESULT hr = S_OK;
    CValue boxedValue;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, item));

    IFC(CoreImports::Collection_Insert(
        static_cast<CCollection*>(GetHandle()),
        index,
        &boxedValue));
            
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<DOUBLE>::Append(_In_ DOUBLE item)
{
    HRESULT hr = S_OK;
    CValue boxedValue;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, item));

    IFC(CoreImports::Collection_Add(
        static_cast<CCollection*>(GetHandle()),
        &boxedValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP 
PresentationFrameworkCollection<DOUBLE>::GetAt(
    _In_opt_ UINT index, 
    _Out_ DOUBLE *item)
{
    HRESULT hr = S_OK;
    CValue value;
    XINT32 nIndex = static_cast<XINT32>(index);
           
    IFC(CheckThread());
    IFC(CoreImports::Collection_GetItem(static_cast<CCollection*>(GetHandle()), nIndex, &value));
    IFC(CValueBoxer::UnboxValue(&value, item));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<DOUBLE>::IndexOf(_In_ DOUBLE value, _Out_ UINT *index, _Out_ BOOLEAN *found)
{
    HRESULT hr = S_FALSE;
    CValue boxedValue;
    XINT32 coreIndex = -1;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, value));
                
    if (SUCCEEDED(CoreImports::Collection_IndexOf(
        static_cast<CCollection*>(GetHandle()),
        &boxedValue,
        &coreIndex)))
    {
        *index = static_cast<UINT>(coreIndex);
    }
    else
    {
        IFC(S_FALSE);
    }

    *found = coreIndex != -1;

Cleanup:      
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<DOUBLE>::InsertAt(_In_ UINT index, _In_ DOUBLE item)
{
    HRESULT hr = S_OK;
    CValue boxedValue;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, item));

    IFC(CoreImports::Collection_Insert(
        static_cast<CCollection*>(GetHandle()),
        index,
        &boxedValue));
            
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<wf::Point>::Append(_In_ wf::Point item)
{
    HRESULT hr = S_OK;
    CValue boxedValue;
    BoxerBuffer buffer;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, item, &buffer));

    IFC(CoreImports::Collection_Add(
        static_cast<CCollection*>(GetHandle()),
        &boxedValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP 
PresentationFrameworkCollection<wf::Point>::GetAt(
    _In_opt_ UINT index, 
    _Out_ wf::Point *item)
{
    HRESULT hr = S_OK;
    CValue value;
    XINT32 nIndex = static_cast<XINT32>(index);

    IFC(CheckThread());
    IFC(CoreImports::Collection_GetItem(static_cast<CCollection*>(GetHandle()), nIndex, &value));
    IFC(CValueBoxer::UnboxValue(&value, item));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<wf::Point>::IndexOf(_In_ wf::Point value, _Out_ UINT *index, _Out_ BOOLEAN *found)
{
    HRESULT hr = S_FALSE;
    CValue boxedValue;
    BoxerBuffer buffer;
    XINT32 coreIndex = -1;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, value, &buffer));

    if (SUCCEEDED(CoreImports::Collection_IndexOf(
        static_cast<CCollection*>(GetHandle()),
        &boxedValue,
        &coreIndex)))
    {
        *index = static_cast<UINT>(coreIndex);
    }
    else
    {
        IFC(S_FALSE);
    }

    *found = coreIndex != -1;

Cleanup:      
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<wf::Point>::InsertAt(_In_ UINT index, _In_ wf::Point item)
{
    HRESULT hr = S_OK;
    CValue boxedValue;
    BoxerBuffer buffer;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, item, &buffer));

    IFC(CoreImports::Collection_Insert(
        static_cast<CCollection*>(GetHandle()),
        index,
        &boxedValue));
            
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<xaml_docs::TextRange>::Append(_In_ xaml_docs::TextRange item)
{
    HRESULT hr = S_OK;
    CValue boxedValue;
    BoxerBuffer buffer;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, item, &buffer));

    IFC(CoreImports::Collection_Add(
        static_cast<CCollection*>(GetHandle()),
        &boxedValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<xaml_docs::TextRange>::GetAt(
    _In_opt_ UINT index,
    _Out_ xaml_docs::TextRange *item)
{
    HRESULT hr = S_OK;
    CValue value;
    XINT32 nIndex = static_cast<XINT32>(index);

    IFC(CheckThread());
    IFC(CoreImports::Collection_GetItem(static_cast<CCollection*>(GetHandle()), nIndex, &value));
    IFC(CValueBoxer::UnboxValue(&value, item));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<xaml_docs::TextRange>::IndexOf(_In_ xaml_docs::TextRange value, _Out_ UINT *index, _Out_ BOOLEAN *found)
{
    HRESULT hr = S_FALSE;
    CValue boxedValue;
    BoxerBuffer buffer;
    XINT32 coreIndex = -1;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, value, &buffer));

    if (SUCCEEDED(CoreImports::Collection_IndexOf(
        static_cast<CCollection*>(GetHandle()),
        &boxedValue,
        &coreIndex)))
    {
        *index = static_cast<UINT>(coreIndex);
    }
    else
    {
        IFC(S_FALSE);
    }

    *found = coreIndex != -1;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
PresentationFrameworkCollection<xaml_docs::TextRange>::InsertAt(_In_ UINT index, _In_ xaml_docs::TextRange item)
{
    HRESULT hr = S_OK;
    CValue boxedValue;
    BoxerBuffer buffer;

    IFC(CheckThread());
    IFC(CValueBoxer::BoxValue(&boxedValue, item, &buffer));

    IFC(CoreImports::Collection_Insert(
        static_cast<CCollection*>(GetHandle()),
        index,
        &boxedValue));

Cleanup:
    RRETURN(hr);
}

namespace DirectUI
{
    bool UntypedTryGetIndexOf(_In_ IUntypedVector* vector, _In_ IInspectable* item, _Out_ unsigned int* index)
    {
        *index = 0;
        wrl::ComPtr<IInspectable> itemIdentity;
        IFCFAILFAST(item->QueryInterface(itemIdentity.ReleaseAndGetAddressOf()));

        bool found = false;
        unsigned int size = 0, untypedIndex = 0;
        IFCFAILFAST(vector->UntypedGetSize(&size));
        while (untypedIndex < size && !found)
        {
            wrl::ComPtr<IInspectable> itemAt;
            IFCFAILFAST(vector->UntypedGetAt(untypedIndex, &itemAt));
            found = (itemAt == itemIdentity);
            ++untypedIndex;
        }

        if (found)
        {
            *index = untypedIndex - 1;
        }

        return found;
    }
}

#pragma endregion
