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

#include "ContentPresenter.g.h"

#define __ListViewBaseItemPresenter_GUID "ce067044-972a-4fd8-b884-de2d3f2d411d"

namespace DirectUI
{
    class ListViewBaseItemPresenter;

    class __declspec(novtable) ListViewBaseItemPresenterGenerated:
        public DirectUI::ContentPresenter
    {
        friend class DirectUI::ListViewBaseItemPresenter;



    public:
        ListViewBaseItemPresenterGenerated();
        ~ListViewBaseItemPresenterGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ListViewBaseItemPresenter;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ListViewBaseItemPresenter;
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

#include "ListViewBaseItemPresenter_Partial.h"

