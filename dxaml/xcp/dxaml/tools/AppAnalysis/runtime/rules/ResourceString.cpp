// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <strsafe.h>
#include "ResourceString.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

IFACEMETHODIMP
ResourceStringFactory::CreateInstance(
    _In_ UINT32 identifier,
    _COM_Outptr_ appanalysis::IResourceString** resourceString
    )
{
    IFCPTR_RETURN(resourceString);
    *resourceString = nullptr;

    IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(resourceString, identifier));
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
HRESULT
ResourceString::RuntimeClassInitialize(_In_ UINT32 identifier)
{
    IFC_RETURN(wfci_::Vector<HSTRING>::Make(m_args.GetAddressOf()));
    m_identifier = identifier;
    return S_OK;
}

IFACEMETHODIMP
ResourceString::get_Identifier(
    _Out_ UINT32* identifier
    )
{
    ARG_VALIDRETURNPOINTER(identifier);
    *identifier = m_identifier;
    return S_OK;
}

IFACEMETHODIMP
ResourceString::put_Identifier(
    _In_ UINT32 identifier
    )
{
    m_identifier = identifier;
    return S_OK;
}

IFACEMETHODIMP
ResourceString::GetResourceStringView(
    _COM_Outptr_ appanalysis::IResourceStringView** view
    )
{
    IFCPTR_RETURN(view);
    *view = nullptr;
    IFC_RETURN(wrl::MakeAndInitialize<ResourceStringView>(view, this));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::First(
    _COM_Outptr_ wfc::IIterator<HSTRING> **iterator)
{
    ARG_VALIDRETURNPOINTER(iterator);

    *iterator = nullptr;
    IFC_RETURN(m_args->First(iterator));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::GetAt(
    _In_ unsigned int index,
    _Outptr_ HSTRING* arg)
{
    ARG_VALIDRETURNPOINTER(arg);

    IFC_RETURN(m_args->GetAt(index, arg));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::get_Size(
    _Out_ unsigned int *size)
{
    ARG_VALIDRETURNPOINTER(size);
    *size = 0;
     IFC_RETURN(m_args->get_Size(size));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::IndexOf(
    _In_ HSTRING arg,
    _Out_ unsigned int *index,
    _Out_ boolean *found)
{
    ARG_VALIDRETURNPOINTER(index);
    ARG_VALIDRETURNPOINTER(found);

    IFC_RETURN(m_args->IndexOf(arg, index, found));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::GetView(
    _COM_Outptr_result_maybenull_ wfc::IVectorView<HSTRING> **view)
{
    ARG_VALIDRETURNPOINTER(view);
    *view = nullptr;

     IFC_RETURN(m_args->GetView(view));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::SetAt(
    _In_ unsigned index,
    _In_opt_ HSTRING item
    )
{
    IFC_RETURN(m_args->SetAt(index, item));
    return S_OK;

}

IFACEMETHODIMP
ResourceString::InsertAt(
    _In_ unsigned index,
    _In_opt_ HSTRING item
    )
{
    IFC_RETURN(m_args->InsertAt(index, item));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::RemoveAt(
    _In_ unsigned index
    )
{
    IFC_RETURN(m_args->RemoveAt(index));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::Append(
    _In_opt_ HSTRING item
    )
{
    IFC_RETURN(m_args->Append(item));

    return S_OK;
}

IFACEMETHODIMP
ResourceString::RemoveAtEnd()
{
    IFC_RETURN(m_args->RemoveAtEnd());

    return S_OK;
}

IFACEMETHODIMP
ResourceString::Clear()
{
    IFC_RETURN(m_args->Clear());

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// ResourceStringView implementation

HRESULT
ResourceStringView::RuntimeClassInitialize(_In_opt_ appanalysis::IResourceString* string)
{
    if (!string)
    {
        return S_OK;
    }

    IFC_RETURN(string->get_Identifier(&m_identifier));

    wrl::ComPtr<wfc::IVector<HSTRING>> inputArgs;
    IFC_RETURN(AppAnalysisHelpers::As(string, &inputArgs));
    IFC_RETURN(AppAnalysisHelpers::As(string, &m_iterable));
    IFC_RETURN(inputArgs->GetView(&m_args));
    return S_OK;
}

IFACEMETHODIMP
ResourceStringView::get_Identifier(
    _Out_ UINT32* identifier
    )
{
    ARG_VALIDRETURNPOINTER(identifier);
    *identifier = m_identifier;
    return S_OK;
}


IFACEMETHODIMP
ResourceStringView::First(
    _COM_Outptr_ wfc::IIterator<HSTRING> **iterator)
{
    ARG_VALIDRETURNPOINTER(iterator);

    *iterator = nullptr;
    if (m_iterable)
    {
        IFC_RETURN(m_iterable->First(iterator));
    }

    return S_OK;
}

IFACEMETHODIMP
ResourceStringView::GetAt(
    _In_ unsigned int index,
    _Outptr_ HSTRING* arg)
{
    ARG_VALIDRETURNPOINTER(arg);
    IFC_RETURN(CheckBounds());
    IFC_RETURN(m_args->GetAt(index, arg));

    return S_OK;
}

IFACEMETHODIMP
ResourceStringView::get_Size(
    _Out_ unsigned int *size)
{
    ARG_VALIDRETURNPOINTER(size);
    *size = 0;
    if (m_args)
    {
        IFC_RETURN(m_args->get_Size(size));
    }

    return S_OK;
}

IFACEMETHODIMP
ResourceStringView::IndexOf(
    _In_ HSTRING arg,
    _Out_ unsigned int *index,
    _Out_ boolean *found)
{
    ARG_VALIDRETURNPOINTER(index);
    ARG_VALIDRETURNPOINTER(found);
    IFC_RETURN(CheckBounds());
    IFC_RETURN(m_args->IndexOf(arg, index, found));

    return S_OK;
}

HRESULT
ResourceStringView::CheckBounds()
{
    return m_args ? S_OK : E_BOUNDS;
}

} } }
