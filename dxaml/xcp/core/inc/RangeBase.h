// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ccontrol.h>
#include <limits>

class CRangeBase : public CControl
{
protected:
    CRangeBase(
        _In_ CCoreServices *core)
    : CControl(core)
    {}

public:
    DECLARE_CREATE(CRangeBase);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::RangeBase;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT SetValue(
        _In_ const SetValueParams& args) override;

public:
    double m_minimum = 0.0;
    double m_maximum = 1.0;
    double m_value = 0.0;

private:
    double m_uncoercedValue = 0.0;

    _Check_return_ HRESULT GetDoubleValueHelper(
        KnownPropertyIndex valueIndex,
        _Out_ double& result);

    _Check_return_ HRESULT SetDoubleValueHelper(
        _In_opt_ const SetValueParams* args,
        KnownPropertyIndex valueIndex,
        double value);

    _Check_return_ HRESULT CoerceAndSetValue(
        _In_opt_ const SetValueParams* args,
        double min,
        double max);

    _Check_return_ HRESULT SetMaximum(
        _In_opt_ const SetValueParams* args,
        double max);
};
