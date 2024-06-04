// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <StateTriggerBase.h>

class CAdaptiveTrigger final : public CStateTriggerBase
{
public:
    // Creation method
    DECLARE_CREATE(CAdaptiveTrigger);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CAdaptiveTrigger>::Index;
    }

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) final;

    XFLOAT m_minWindowWidth     = -1.0f;
    XFLOAT m_minWindowHeight    = -1.0f;

private:
    CAdaptiveTrigger(_In_ CCoreServices *pCore)
        : CStateTriggerBase(pCore)
    {}
};
