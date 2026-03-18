// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FrameworkElementAutomationPeer.h"
#include <DeclareMacros.h>
#include <Indexes.g.h>
#include <minxcptypes.h>
#include "FrameworkUdk/Containment.h"

// Bug 60878987: [1.8 Servicing][WASDK] Add SplitMenuFlyoutItem control
#define WINAPPSDK_CHANGEID_60878987 60878987, WinAppSDK_1_8_6

class CSplitMenuFlyoutItemAutomationPeer : public CFrameworkElementAutomationPeer
{
protected:
    CSplitMenuFlyoutItemAutomationPeer(_In_ CCoreServices *pCore, _In_ CValue &value)
        : CFrameworkElementAutomationPeer(pCore, value)
    {
        SetIsCustomType();
    }

    ~CSplitMenuFlyoutItemAutomationPeer() override = default;

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        )
    {
        HRESULT hr = S_OK;
        CSplitMenuFlyoutItemAutomationPeer* pObject = NULL;

        IFCEXPECT(pCreate);
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_60878987>())
        {
            if (pCreate->m_value.GetType() != valueObject)
            {
                IFC(E_NOTIMPL);
            }
            else
            {
                pObject = new CSplitMenuFlyoutItemAutomationPeer(pCreate->m_pCore, pCreate->m_value);
                hr = ValidateAndInit(pObject, ppObject);
                if (FAILED(hr)) delete pObject;
            }
        }
        else
        {
            IFC(E_NOTIMPL);
        }

    Cleanup:
        RRETURN(hr);
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::SplitMenuFlyoutItemAutomationPeer;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};