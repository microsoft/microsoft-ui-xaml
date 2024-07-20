// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Activators.g.h>

class CIsTextTrimmedChangedEventArgs final : public CEventArgs
{

public:

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override
    {
        RRETURN(DirectUI::OnFrameworkCreateIsTextTrimmedChangedEventArgs(this, ppPeer));
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIsTextTrimmedChangedEventArgs>::Index;
    }

};