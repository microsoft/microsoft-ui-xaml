// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef PASSWORD_BOX_AUTOMATIONPEER_H
#define PASSWORD_BOX_AUTOMATIONPEER_H

#include "TextBoxBaseAutomationPeer.h"

class CPasswordBox;

//---------------------------------------------------------------------------
//
//  CPasswordBox automation peer for accessibility.
//
//---------------------------------------------------------------------------
class CPasswordBoxAutomationPeer : public CTextBoxBaseAutomationPeer
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
        return DependencyObjectTraits<CPasswordBoxAutomationPeer>::Index;
    }

protected:
    CPasswordBoxAutomationPeer(
        _In_ CCoreServices *pCore,
        _In_ CValue        &value);

    ~CPasswordBoxAutomationPeer() override;
};
#endif //PASSWORD_BOX_AUTOMATIONPEER_H
