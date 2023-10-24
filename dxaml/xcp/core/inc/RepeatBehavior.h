// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Duration.h"
#include "RepeatBehaviorVO.h"
#include <type_traits>

class CRepeatBehavior final
    : public CDuration
{
private:
    CRepeatBehavior(_In_ CCoreServices* core)
        : CDuration(core)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CRepeatBehavior(DirectUI::RepeatBehaviorType type, double timeSpan, float count)    // !!! FOR UNIT TESTING ONLY !!!
        : CDuration(static_cast<DirectUI::DurationType>(type), timeSpan)
        , m_count(count)
    {}
#endif

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **result,
        _In_ CREATEPARAMETERS *createParams);

    KnownTypeIndex GetTypeIndex() const override;

    double GetDurationInSec() const
    {
        return m_rTimeSpan;
    }

    void SetDurationInSec(double duration)
    {
        m_rTimeSpan = duration;
    }

    DirectUI::RepeatBehaviorType GetRepeatBehaviorType() const
    {
        return static_cast<DirectUI::RepeatBehaviorType>(m_durationType);
    }

    void SetRepeatBehaviorType(DirectUI::RepeatBehaviorType type)
    {
        m_durationType = static_cast<DirectUI::DurationType>(type);
    }

    float GetCount() const
    {
        return m_count;
    }

    void SetCount(float count)
    {
        m_count = count;
    }

    xref_ptr<RepeatBehaviorVO::Wrapper> ValueWrapper() const
    {
        return RepeatBehaviorVOHelper::Create(
                GetContext(),
                GetRepeatBehaviorType(),
                GetDurationInSec(),
                GetCount());
    }

    float m_count = 1.0f;

    static_assert(std::is_same<
        std::underlying_type<DirectUI::RepeatBehaviorType>::type,
        std::underlying_type<DirectUI::DurationType>::type>::value,
        "Cannot store RepeatBehaviorType in DurationType.");
};
