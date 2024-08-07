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

#include "Control.g.h"

#define __CalendarViewBaseItem_GUID "c1d6ef7b-c032-40e2-b4c9-1f4651aacc9c"

namespace DirectUI
{
    class CalendarViewBaseItem;

    class __declspec(novtable) CalendarViewBaseItemGenerated:
        public DirectUI::Control
    {
        friend class DirectUI::CalendarViewBaseItem;



    public:
        CalendarViewBaseItemGenerated();
        ~CalendarViewBaseItemGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::CalendarViewBaseItem;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::CalendarViewBaseItem;
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

#include "CalendarViewBaseItem_Partial.h"

