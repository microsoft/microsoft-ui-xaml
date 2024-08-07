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

#include "Panel.g.h"

#define __RootVisual_GUID "11284afe-6b4c-4ddc-acdd-3afeb1c4550a"

namespace DirectUI
{
    class RootVisual;

    class __declspec(novtable) __declspec(uuid(__RootVisual_GUID)) RootVisual:
        public DirectUI::Panel
    {



    public:
        RootVisual();
        ~RootVisual() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::RootVisual;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::RootVisual;
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


