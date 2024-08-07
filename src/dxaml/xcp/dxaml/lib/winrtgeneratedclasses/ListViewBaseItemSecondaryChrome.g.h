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

#include "FrameworkElement.g.h"

#define __ListViewBaseItemSecondaryChrome_GUID "bb3055ca-b3da-4750-94c7-2e621425f4ed"

namespace DirectUI
{
    class ListViewBaseItemSecondaryChrome;

    class __declspec(novtable) ListViewBaseItemSecondaryChromeGenerated:
        public DirectUI::FrameworkElement
    {
        friend class DirectUI::ListViewBaseItemSecondaryChrome;



    public:
        ListViewBaseItemSecondaryChromeGenerated();
        ~ListViewBaseItemSecondaryChromeGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ListViewBaseItemSecondaryChrome;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ListViewBaseItemSecondaryChrome;
        }

        // Properties.

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "ListViewBaseItemSecondaryChrome_Partial.h"

