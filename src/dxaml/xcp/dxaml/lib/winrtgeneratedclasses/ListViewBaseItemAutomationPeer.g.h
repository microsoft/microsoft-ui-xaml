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

#include "FrameworkElementAutomationPeer.g.h"

#define __ListViewBaseItemAutomationPeer_GUID "33968b7b-b3f5-491a-9f3a-1d0ccd473498"

namespace DirectUI
{
    class ListViewBaseItemAutomationPeer;
    class IRawElementProviderSimple;

    class __declspec(novtable) ListViewBaseItemAutomationPeerGenerated:
        public DirectUI::FrameworkElementAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Provider::IDragProvider
    {
        friend class DirectUI::ListViewBaseItemAutomationPeer;


        BEGIN_INTERFACE_MAP(ListViewBaseItemAutomationPeerGenerated, DirectUI::FrameworkElementAutomationPeer)
            INTERFACE_ENTRY(ListViewBaseItemAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Provider::IDragProvider)
        END_INTERFACE_MAP(ListViewBaseItemAutomationPeerGenerated, DirectUI::FrameworkElementAutomationPeer)

    public:
        ListViewBaseItemAutomationPeerGenerated();
        ~ListViewBaseItemAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ListViewBaseItemAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ListViewBaseItemAutomationPeer;
        }

        // Properties.
        IFACEMETHOD(get_DropEffect)(_Out_ HSTRING* pValue) override;
        IFACEMETHOD(get_DropEffects)(_Out_ UINT* pCount, _Out_writes_to_ptr_(*pCount) HSTRING** pValue) override;
        IFACEMETHOD(get_IsGrabbed)(_Out_ BOOLEAN* pValue) override;

        // Events.

        // Methods.
        IFACEMETHOD(GetGrabbedItems)(_Out_ UINT* pReturnValueCount, _Out_writes_to_ptr_(*pReturnValueCount) ABI::Microsoft::UI::Xaml::Automation::Provider::IIRawElementProviderSimple*** ppReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "ListViewBaseItemAutomationPeer_Partial.h"

