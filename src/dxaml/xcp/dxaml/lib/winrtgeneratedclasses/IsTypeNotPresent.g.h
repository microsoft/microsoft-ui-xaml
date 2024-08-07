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

#include "IXamlPredicate.g.h"

#define __IsTypeNotPresent_GUID "e8c566d6-0e2e-414d-81a8-eb1be51a045e"

namespace DirectUI
{
    class IsTypeNotPresent;

    class __declspec(novtable) IsTypeNotPresentGenerated:
        public DirectUI::DependencyObject
        , public DirectUI::IXamlPredicate
    {
        friend class DirectUI::IsTypeNotPresent;



    public:
        IsTypeNotPresentGenerated();
        ~IsTypeNotPresentGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::IsTypeNotPresent;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::IsTypeNotPresent;
        }

        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(Evaluate)(_In_ ABI::Windows::Foundation::Collections::IVectorView<HSTRING>* pArguments, _Out_ BOOLEAN* pReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "IsTypeNotPresent_Partial.h"

