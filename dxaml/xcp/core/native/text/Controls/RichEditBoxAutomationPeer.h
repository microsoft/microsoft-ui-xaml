// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef RICH_EDIT_BOX_AUTOMATIONPEER_H
#define RICH_EDIT_BOX_AUTOMATIONPEER_H

#include "TextBoxBaseAutomationPeer.h"

class CRichEditBox;

//---------------------------------------------------------------------------
//
//  CRichEditBox automation peer for accessibility.
//
//---------------------------------------------------------------------------
class CRichEditBoxAutomationPeer : public CTextBoxBaseAutomationPeer
{
public:
    // Creation method
    _Check_return_ static HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    // CDependencyObject overrides.
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRichEditBoxAutomationPeer>::Index;
    }

protected:
    CRichEditBoxAutomationPeer(
        _In_ CCoreServices *pCore,
        _In_ CValue        &value);

    ~CRichEditBoxAutomationPeer() override;
};
#endif //RICH_EDIT_BOX_AUTOMATIONPEER_H
