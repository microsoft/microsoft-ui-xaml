// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RepeatBehaviorVO.h"
#include "FlyweightCoreAdapter.h"
#include <CommonUtilities.h>

namespace Flyweight
{
    namespace Operators
    {
        template <>
        RepeatBehaviorVO Default()
        {
            return RepeatBehaviorVO(
                RepeatBehaviorVO::s_defaultDurationInSec,
                RepeatBehaviorVO::s_defaultRepeatBehaviorType,
                RepeatBehaviorVO::s_defaultCount);
        }

        template <>
        bool equal(
            _In_ const RepeatBehaviorVO& lhs,
            _In_ const RepeatBehaviorVO& rhs)
        {
            return std::tie(lhs.m_type, lhs.m_durationInSec, lhs.m_count) ==
                   std::tie(rhs.m_type, rhs.m_durationInSec, rhs.m_count);
        }

        template <>
        bool less(
            _In_ const RepeatBehaviorVO& lhs,
            _In_ const RepeatBehaviorVO& rhs)
        {
            return std::tie(lhs.m_type, lhs.m_durationInSec, lhs.m_count) <
                   std::tie(rhs.m_type, rhs.m_durationInSec, rhs.m_count);
        }

        template <>
        std::size_t hash(
            _In_ const RepeatBehaviorVO& inst)
        {
            std::size_t hash = 0;
            CommonUtilities::hash_combine(hash, inst.GetRepeatBehaviorType());
            CommonUtilities::hash_combine(hash, inst.GetDurationInSec());
            CommonUtilities::hash_combine(hash, inst.GetCount());
            return hash;
        }
    }
}

bool operator==(
    _In_ const RepeatBehaviorVO& lhs,
    _In_ const RepeatBehaviorVO& rhs)
{
    return Flyweight::Operators::equal(lhs, rhs);
}

namespace RepeatBehaviorVOHelper
{
    xref_ptr<RepeatBehaviorVO::Wrapper> Create(
        _In_opt_ CCoreServices* core,
        DirectUI::RepeatBehaviorType type,
        double durationInSec,
        float count)
    {
        switch(type)
        {
            default:
                // Default is Count, but raise assert and fall-through...
                ASSERT(false);

            case DirectUI::RepeatBehaviorType::Count:
                return CreateCount(core, count);

            case DirectUI::RepeatBehaviorType::Duration:
                return CreateDuration(core, durationInSec);

            case DirectUI::RepeatBehaviorType::Forever:
                return CreateForever(core);
        }
    }

    xref_ptr<RepeatBehaviorVO::Wrapper> CreateCount(
        _In_opt_ CCoreServices* core,
        float count)
    {
        return Flyweight::Create<RepeatBehaviorVO>(
            core,
            RepeatBehaviorVO::s_defaultDurationInSec,
            DirectUI::RepeatBehaviorType::Count,
            count);
    }

    xref_ptr<RepeatBehaviorVO::Wrapper> CreateDuration(
        _In_opt_ CCoreServices* core,
        double durationInSec)
    {
        return Flyweight::Create<RepeatBehaviorVO>(
            core,
            durationInSec,
            DirectUI::RepeatBehaviorType::Duration,
            RepeatBehaviorVO::s_defaultCount);
    }

    xref_ptr<RepeatBehaviorVO::Wrapper> CreateForever(
        _In_opt_ CCoreServices* core)
    {
        return Flyweight::Create<RepeatBehaviorVO>(
            core,
            RepeatBehaviorVO::s_defaultDurationInSec,
            DirectUI::RepeatBehaviorType::Forever,
            RepeatBehaviorVO::s_defaultCount);
    }

    bool IsZeroLength(
        _In_ const RepeatBehaviorVO& value)
    {
        return (value.GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Count && value.GetCount() == 0.0f) ||
               (value.GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Duration && value.GetDurationInSec() == 0.0);
    }

    double GetTotalDuration(
        const RepeatBehaviorVO& value,
        double iterationDuration,
        _Out_ bool* pLoopsForever)
    {
        bool loopsForever = false;
        double totalDuration = 0;

        switch (value.GetRepeatBehaviorType())
        {
            case DirectUI::RepeatBehaviorType::Count:
                totalDuration = iterationDuration * static_cast<double>(value.GetCount());
                break;

            case DirectUI::RepeatBehaviorType::Duration:
                totalDuration = value.GetDurationInSec();
                break;

            case DirectUI::RepeatBehaviorType::Forever:
                loopsForever = true;
                break;
        }

        *pLoopsForever = loopsForever;

        return totalDuration;
    }

    bool RepeatsOnce(
        _In_ const RepeatBehaviorVO& value)
    {
        return value.GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Count &&
            value.GetCount() == 1.0f;
    }

    bool RepeatsForever(
        _In_ const RepeatBehaviorVO& value)
    {
        return value.GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Forever;
    }

    float GetRepeatCount(
        _In_ const RepeatBehaviorVO& value,
        float naturalDuration)
    {
        switch (value.GetRepeatBehaviorType())
        {
            case DirectUI::RepeatBehaviorType::Forever:
                ASSERT(false);
                return 0.0f;

            case DirectUI::RepeatBehaviorType::Duration:
                return static_cast<float>(value.GetDurationInSec()) / naturalDuration;

            case DirectUI::RepeatBehaviorType::Count:
            default:
                return value.GetCount();
        }
    }
}