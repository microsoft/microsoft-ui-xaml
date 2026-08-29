// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TimeSpan.h"
#include "DurationVO.h"

class CDuration
    : public CTimeSpan
{
protected:
    CDuration(_In_ CCoreServices* core)
        : CTimeSpan(core)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CDuration(DirectUI::DurationType durationType, double timeSpan) // !!! FOR UNIT TESTING ONLY !!!
        : CTimeSpan(timeSpan)
        , m_durationType(durationType)
    {}
#endif

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **result,
        _In_ CREATEPARAMETERS *createParams);

    KnownTypeIndex GetTypeIndex() const override;

    double GetTimeSpanInSec() const
    {
        return m_rTimeSpan;
    }

    void SetTimeSpanInSec(double timeSpan)
    {
        m_rTimeSpan = timeSpan;
    }

    DirectUI::DurationType GetDurationType() const
    {
        return m_durationType;
    }

    void SetDurationType(DirectUI::DurationType type)
    {
        m_durationType = type;
    }

    xref_ptr<DurationVO::Wrapper> ValueWrapper() const
    {
        return DurationVOHelper::Create(
            GetContext(),
            GetDurationType(),
            GetTimeSpanInSec());
    }

    DirectUI::DurationType m_durationType = DirectUI::DurationType::Automatic;
};