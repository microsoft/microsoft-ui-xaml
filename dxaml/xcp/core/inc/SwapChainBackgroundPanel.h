// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SwapChainPanel.h"

class CSwapChainBackgroundPanel final : public CSwapChainPanel
{
protected:
    CSwapChainBackgroundPanel(_In_ CCoreServices *pCore);
    ~CSwapChainBackgroundPanel() override;

public:
    KnownTypeIndex GetTypeIndex() const final
    {
        return DependencyObjectTraits<CSwapChainBackgroundPanel>::Index;
    }

    DECLARE_CREATE(CSwapChainBackgroundPanel);

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT PreRender();

    XUINT32 ParticipatesInManagedTreeInternal() final
    {
        // SwapChainBackgroundPanel needs to participate in DXaml tree to enable
        // parent validation checks via CDependencyObject::SetParent()

        return PARTICIPATES_IN_MANAGED_TREE;
    }

    //
    // SwapChainBackgroundPanel needs to ensure its DXaml peer is created when its tree parent is being updated.
    // This ensures SCBP code in DXaml layer gets a chance to validate that the parent type is a Page element.
    //
    _Check_return_ HRESULT PreTreeParentUpdated(_In_opt_ CDependencyObject *pNewParent)
    {
        if (pNewParent)
        {
            IFC_RETURN(PegManagedPeer());
        }

        return S_OK;
    }

    //
    // Unpeg the peer once parent validation work is done.
    //
    void PostTreeParentUpdated(_In_opt_ CDependencyObject *pNewParent)
    {
        if (pNewParent)
        {
            UnpegManagedPeer();
        }
    }
};
