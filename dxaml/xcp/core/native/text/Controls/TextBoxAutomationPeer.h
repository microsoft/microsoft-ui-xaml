// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef TEXT_BOX_AUTOMATIONPEER_H
#define TEXT_BOX_AUTOMATIONPEER_H

#include "TextBoxBaseAutomationPeer.h"

class CTextBox;

//---------------------------------------------------------------------------
//
//  CTextBox automation peer for accessibility.
//
//---------------------------------------------------------------------------
class CTextBoxAutomationPeer : public CTextBoxBaseAutomationPeer
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
        return DependencyObjectTraits<CTextBoxAutomationPeer>::Index;
    }

protected:
    CTextBoxAutomationPeer(
        _In_ CCoreServices *pCore,
        _In_ CValue        &value);

    ~CTextBoxAutomationPeer() override;
};
#endif // TEXT_BOX_AUTOMATIONPEER_H
