// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Image.g.h"
#include <fwd/windows.media.h>

namespace DirectUI
{
    PARTIAL_CLASS(Image)
    {
    public:

        // Initializes a new instance of the Image class.
        Image();

        // Destroys an instance of the Image class.
        ~Image() override;

        _Check_return_ HRESULT GetAsCastingSourceImpl(_Outptr_ wm::Casting::ICastingSource** ppReturnValue);

        _Check_return_ HRESULT GetAlphaMaskImpl(_Outptr_ WUComp::ICompositionBrush** ppResult);

    protected:
        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

    private:
        wm::Casting::ICastingSource *m_pCastingSource;
    };
}
