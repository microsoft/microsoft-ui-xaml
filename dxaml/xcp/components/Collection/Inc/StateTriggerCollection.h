// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "VariantMap.h"

#pragma once

class CStateTriggerCollection final : public CDOCollection
{
public:
    // Creation method
    DECLARE_CREATE(CStateTriggerCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CStateTriggerCollection>::Index;
    }

    std::shared_ptr<StateTriggerVariantMap> m_pVariantMap;

    // DOCollection overrides
    _Check_return_ HRESULT Insert(XUINT32 index, _In_ CDependencyObject* object) final;
    _Check_return_ HRESULT Append(_In_ CDependencyObject* object, _Out_opt_ unsigned int* index = nullptr) final;
    _Check_return_ HRESULT OnClear() final;
    _Check_return_ void* RemoveAt(XUINT32 nIndex) final;

    // Required for GetNamescopeParent to succeed in CanProcessEnterLeave. If ShouldEnsureNameResolution is false,
    // collection owner must be manually set with SetOwner to enable Enter/Leave on collection items. This is
    // unintuitive and the methods should be renamed or revised.
    bool ShouldEnsureNameResolution() override { return true; }

private:
    CStateTriggerCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

    _Check_return_ HRESULT AddTriggerToVariantMap(_In_ CDependencyObject* trigger);
    _Check_return_ HRESULT RemoveTriggerFromVariantMap(_In_ CDependencyObject* trigger);
    std::shared_ptr<IQualifier> CreateQualifierFromTrigger(_In_ CDependencyObject* trigger);
};
