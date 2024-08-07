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


#define __TextPointerWrapper_GUID "b087bd24-afb0-4aa1-a675-d3465e2114db"

namespace DirectUI
{
    class TextPointerWrapper;

    class __declspec(novtable) __declspec(uuid(__TextPointerWrapper_GUID)) TextPointerWrapper:
        public DirectUI::DependencyObject
    {



    public:
        TextPointerWrapper();
        ~TextPointerWrapper() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::TextPointerWrapper;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::TextPointerWrapper;
        }

        // Properties.

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


