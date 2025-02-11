// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "precomp.h"
#include "PrintPageEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::PrintPageEventArgs::PrintPageEventArgs(): m_hasMorePages(), m_printableArea(), m_pageMargins()
{
}

DirectUI::PrintPageEventArgs::~PrintPageEventArgs()
{
}

HRESULT DirectUI::PrintPageEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PrintPageEventArgs)))
    {
        *ppObject = static_cast<DirectUI::PrintPageEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
_Check_return_ HRESULT DirectUI::PrintPageEventArgs::get_PageVisual(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_pPageVisual.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintPageEventArgs::put_PageVisual(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_pPageVisual, pValue);
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintPageEventArgs::get_HasMorePages(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_hasMorePages, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintPageEventArgs::put_HasMorePages(BOOLEAN value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_hasMorePages));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintPageEventArgs::get_PrintableArea(_Out_ ABI::Windows::Foundation::Size* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_printableArea, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PrintPageEventArgs::get_PageMargins(_Out_ ABI::Microsoft::UI::Xaml::Thickness* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_pageMargins, pValue));
Cleanup:
    RRETURN(hr);
}

// Methods.


