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

#include "ScrollViewerAutomationPeer.g.h"

#define __CalendarScrollViewerAutomationPeer_GUID "18f3437f-531d-4fe6-b28e-bdbf10b47126"

namespace DirectUI
{
    class CalendarScrollViewerAutomationPeer;

    class __declspec(novtable) CalendarScrollViewerAutomationPeerGenerated:
        public DirectUI::ScrollViewerAutomationPeer
    {
        friend class DirectUI::CalendarScrollViewerAutomationPeer;



    public:
        CalendarScrollViewerAutomationPeerGenerated();
        ~CalendarScrollViewerAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::CalendarScrollViewerAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::CalendarScrollViewerAutomationPeer;
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

#include "CalendarScrollViewerAutomationPeer_Partial.h"

