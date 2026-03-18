// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CMenuFlyoutItem.g.h"
#include "FrameworkUdk/Containment.h"

// Bug 60878987: [1.8 Servicing][WASDK] Add SplitMenuFlyoutItem control
#define WINAPPSDK_CHANGEID_60878987 60878987, WinAppSDK_1_8_6

// #include <DeclareMacros.h>
// #include <Indexes.g.h>
// #include <minxcptypes.h>

class CSplitMenuFlyoutItem : public CMenuFlyoutItem
{
protected:
    CSplitMenuFlyoutItem(_In_ CCoreServices *pCore)
        : CMenuFlyoutItem(pCore)
    {
        SetIsCustomType();
    }

    ~CSplitMenuFlyoutItem() override = default;

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params) final;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params) final;


public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        )
    {
        HRESULT hr = S_OK;

        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_60878987>())
        {
            if (pCreate->m_value.GetType() == valueString)
            {
                IFC(E_NOTIMPL);
            }
            else
            {
                CSplitMenuFlyoutItem *pObj = new CSplitMenuFlyoutItem(pCreate->m_pCore);
                hr = ValidateAndInit(pObj, ppObject);
                if (FAILED(hr)) delete pObj;
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
        return KnownTypeIndex::SplitMenuFlyoutItem;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
