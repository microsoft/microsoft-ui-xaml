// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ValidationErrorsCollection.g.h"
#include "VectorChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


IFACEMETHODIMP ValidationErrorsCollection::SetAt(_In_ uint32_t index, _In_ IInputValidationError* item)
{
    uint32_t size = 0;
    IFC_RETURN(get_Size(&size));
    IFCEXPECTRC_RETURN(index <= size, E_BOUNDS);

    IFC_RETURN(ValidationErrorsCollectionGenerated::SetAt(index, item));
    IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_ItemChanged, index));

    return S_OK;
}

IFACEMETHODIMP ValidationErrorsCollection::InsertAt(_In_ uint32_t index, _In_ IInputValidationError* item)
{
    uint32_t size = 0;
    IFC_RETURN(get_Size(&size));
    IFCEXPECTRC_RETURN(index <= size, E_BOUNDS);

    IFC_RETURN(ValidationErrorsCollectionGenerated::InsertAt(index, item));
    IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index));

    return S_OK;
}

IFACEMETHODIMP ValidationErrorsCollection::RemoveAt(_In_ uint32_t index)
{
    uint32_t size = 0;
    IFC_RETURN(get_Size(&size));
    IFCEXPECTRC_RETURN(index <= size, E_BOUNDS);

    IFC_RETURN(ValidationErrorsCollectionGenerated::RemoveAt(index));
    IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, index));

    return S_OK;
}

IFACEMETHODIMP ValidationErrorsCollection::Append(_In_ IInputValidationError* item)
{
    uint32_t insertedIndex = 0;
    IFC_RETURN(get_Size(&insertedIndex));
    IFC_RETURN(ValidationErrorsCollectionGenerated::Append(item));
    IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, insertedIndex));

    return S_OK;
}

IFACEMETHODIMP ValidationErrorsCollection::RemoveAtEnd()
{
    uint32_t size = 0;

    IFC_RETURN(get_Size(&size));
    IFCEXPECTRC_RETURN(size > 0, E_BOUNDS);

    IFC_RETURN(RemoveAt(size - 1));

    return S_OK;
}

IFACEMETHODIMP ValidationErrorsCollection::Clear()
{
    IFC_RETURN(ValidationErrorsCollectionGenerated::Clear());
    IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));

    return S_OK;
}

_Check_return_ HRESULT ValidationErrorsCollection::RaiseVectorChanged(
    _In_ wfc::CollectionChange change,
    _In_ uint32_t changeIndex)
{
    ctl::ComPtr<VectorChangedEventSourceType> eventSource;
    ctl::ComPtr<VectorChangedEventArgs> args;

    IFC_RETURN(ctl::make(&args));
    IFC_RETURN(args->put_CollectionChange(change));
    IFC_RETURN(args->put_Index(changeIndex));

    IFC_RETURN(GetVectorChangedEventSource(&eventSource));
    IFC_RETURN(eventSource->Raise(this, args.Get()));

    return S_OK;
}
