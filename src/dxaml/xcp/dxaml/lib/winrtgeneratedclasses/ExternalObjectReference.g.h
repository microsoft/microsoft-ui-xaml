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


#define __ExternalObjectReference_GUID "dc678962-0653-4c92-92cd-95943d593900"

namespace DirectUI
{
    class ExternalObjectReference;

    class __declspec(novtable) ExternalObjectReferenceGenerated:
        public DirectUI::DependencyObject
    {
        friend class DirectUI::ExternalObjectReference;



    public:
        ExternalObjectReferenceGenerated();
        ~ExternalObjectReferenceGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ExternalObjectReference;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ExternalObjectReference;
        }

        // Properties.
        _Check_return_ HRESULT get_MarkupExtensionType(_Out_ DirectUI::MarkupExtensionType* pValue);
        _Check_return_ HRESULT put_MarkupExtensionType(DirectUI::MarkupExtensionType value);
        _Check_return_ HRESULT get_NativeValue(_Outptr_result_maybenull_ IInspectable** ppValue);
        _Check_return_ HRESULT put_NativeValue(_In_opt_ IInspectable* pValue);

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "ExternalObjectReference_Partial.h"

