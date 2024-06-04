// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TimeSpan.h"
#include "KeyTimeVO.h"

class CKeyTime final
    : public CTimeSpan
{
private:
    CKeyTime(_In_ CCoreServices* core)
        : CTimeSpan(core)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CKeyTime(double timeSpan) // !!! FOR UNIT TESTING ONLY !!!
        : CTimeSpan(timeSpan)
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

    float GetPercent() const
    {
        return m_percent;
    }

    void SetPercent(float percent)
    {
        m_percent = percent;
    }

    xref_ptr<KeyTimeVO::Wrapper> ValueWrapper() const
    {
        return KeyTimeVOHelper::Create(
                    GetContext(),
                    GetTimeSpanInSec(),
                    GetPercent());
    }

    float m_percent = 0.0f;
};
