// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MultiParentShareableDependencyObject.h"

class CStateTriggerBase : public CMultiParentShareableDependencyObject
{
public:
    // Creation method
    DECLARE_CREATE(CStateTriggerBase);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CStateTriggerBase>::Index;
    }

    bool m_triggerState    = false;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    std::vector<std::weak_ptr<StateTriggerVariantMap>> m_owningVariantMaps;

protected:
    CStateTriggerBase(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {}

    _Check_return_ HRESULT EvaluateStateTriggers();
};
