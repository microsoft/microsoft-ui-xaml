// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DurationVO.h"
#include "FlyweightCoreAdapter.h"
#include <CommonUtilities.h>

namespace Flyweight
{
    namespace Operators
    {
        template <>
        DurationVO Default()
        {
            return DurationVO(
                DurationVO::s_defaultTimeSpanInSec,
                DurationVO::s_defaultDurationType);
        }

        template <>
        bool equal(
            _In_ const DurationVO& lhs,
            _In_ const DurationVO& rhs)
        {
            return std::tie(lhs.m_type, lhs.m_timeSpanInSec) ==
                   std::tie(rhs.m_type, rhs.m_timeSpanInSec);
        }

        template <>
        bool less(
            _In_ const DurationVO& lhs,
            _In_ const DurationVO& rhs)
        {
            return std::tie(lhs.m_type, lhs.m_timeSpanInSec) <
                   std::tie(rhs.m_type, rhs.m_timeSpanInSec);
        }

        template <>
        std::size_t hash(
            _In_ const DurationVO& inst)
        {
            std::size_t hash = 0;
            CommonUtilities::hash_combine(hash, inst.GetDurationType());
            CommonUtilities::hash_combine(hash, inst.GetTimeSpanInSec());
            return hash;
        }
    }
}

bool operator==(
    _In_ const DurationVO& lhs,
    _In_ const DurationVO& rhs)
{
    return Flyweight::Operators::equal(lhs, rhs);
}

namespace DurationVOHelper
{
    xref_ptr<DurationVO::Wrapper> Create(
        _In_opt_ CCoreServices* core,
        DirectUI::DurationType type,
        double timeSpanInSec)
    {
        switch(type)
        {
            case DirectUI::DurationType::TimeSpan:
                return CreateTimeSpan(core, timeSpanInSec);

            default:
                // Default is Automatic, but raise assert and fall-through...
                ASSERT(false);

            case DirectUI::DurationType::Automatic:
                return CreateAutomatic(core);

            case DirectUI::DurationType::Forever:
                return CreateForever(core);
        }
    }

    xref_ptr<DurationVO::Wrapper> CreateTimeSpan(
        _In_opt_ CCoreServices* core,
        double timeSpanInSec)
    {
        return Flyweight::Create<DurationVO>(
                   core,
                   timeSpanInSec,
                   DirectUI::DurationType::TimeSpan);
    }

    xref_ptr<DurationVO::Wrapper> CreateAutomatic(
        _In_opt_ CCoreServices* core)
    {
        return Flyweight::Create<DurationVO>(
                   core,
                   DurationVO::s_defaultTimeSpanInSec,
                   DirectUI::DurationType::Automatic);
    }

    xref_ptr<DurationVO::Wrapper> CreateForever(
        _In_opt_ CCoreServices* core)
    {
        return Flyweight::Create<DurationVO>(
                   core,
                   DurationVO::s_defaultTimeSpanInSec,
                   DirectUI::DurationType::Forever);
    }

    bool IsZeroLength(
        _In_ const DurationVO& value)
    {
        return value.GetDurationType() == DirectUI::DurationType::TimeSpan && value.GetTimeSpanInSec() == 0.0;
    }
}