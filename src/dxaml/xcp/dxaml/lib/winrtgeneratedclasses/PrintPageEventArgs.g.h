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

#pragma once

#include "UIElement.g.h"
#include "Thickness.g.h"

#define __PrintPageEventArgs_GUID "735cac02-30fa-4c19-ac3d-45464e9aba8a"

namespace DirectUI
{
    class PrintPageEventArgs;

    class __declspec(novtable) __declspec(uuid(__PrintPageEventArgs_GUID)) PrintPageEventArgs :
        public DirectUI::EventArgs
    {



    public:
        PrintPageEventArgs();
        ~PrintPageEventArgs() override;

        // Properties.
        _Check_return_ HRESULT get_PageVisual(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue);
        _Check_return_ HRESULT put_PageVisual(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue);
        _Check_return_ HRESULT get_HasMorePages(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_HasMorePages(BOOLEAN value);
        _Check_return_ HRESULT get_PrintableArea(_Out_ ABI::Windows::Foundation::Size* pValue);
        _Check_return_ HRESULT get_PageMargins(_Out_ ABI::Microsoft::UI::Xaml::Thickness* pValue);

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
        TrackerPtr<ABI::Microsoft::UI::Xaml::IUIElement> m_pPageVisual;
        BOOLEAN m_hasMorePages;
        ABI::Windows::Foundation::Size m_printableArea;
        ABI::Microsoft::UI::Xaml::Thickness m_pageMargins;
    };
}


