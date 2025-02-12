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

#include "CalendarViewBaseItem.g.h"

#define __CalendarViewItem_GUID "274b7c18-04f5-411a-8e73-25bea5e12a79"

namespace DirectUI
{
    class CalendarViewItem;

    class __declspec(novtable) CalendarViewItemGenerated:
        public DirectUI::CalendarViewBaseItem
    {
        friend class DirectUI::CalendarViewItem;



    public:
        CalendarViewItemGenerated();
        ~CalendarViewItemGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::CalendarViewItem;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::CalendarViewItem;
        }

        // Properties.
        _Check_return_ HRESULT get_Date(_Out_ ABI::Windows::Foundation::DateTime* pValue);
        virtual _Check_return_ HRESULT put_Date(ABI::Windows::Foundation::DateTime value);

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "CalendarViewItem_Partial.h"

