// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CMenuFlyout : public CFlyoutBase
{
protected:
    CMenuFlyout(_In_ CCoreServices *pCore)
        : CFlyoutBase(pCore)
    {
        SetIsCustomType();
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

public:
    DECLARE_CREATE(CMenuFlyout);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::MenuFlyout;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    static _Check_return_ HRESULT KeyboardAcceleratorFlyoutItemEnter(
        _In_ CDependencyObject* element,
        _In_ CDependencyObject *pNamescopeOwner,
        _In_ KnownPropertyIndex collectionPropertyIndex,
        _In_ EnterParams params);

    static _Check_return_ HRESULT KeyboardAcceleratorFlyoutItemLeave(
        _In_ CDependencyObject* element,
        _In_ CDependencyObject *pNamescopeOwner,
        _In_ KnownPropertyIndex collectionPropertyIndex,
        _In_ LeaveParams params);
};