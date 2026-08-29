// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyTimeVO.h"
#include "FlyweightCoreAdapter.h"
#include <CommonUtilities.h>

namespace Flyweight
{
    namespace Operators
    {
        template <>
        KeyTimeVO Default()
        {
            return KeyTimeVO(
                KeyTimeVO::s_defaultTimeSpanInSec,
                KeyTimeVO::s_defaultPercent);
        }

        template <>
        bool equal(
            _In_ const KeyTimeVO& lhs,
            _In_ const KeyTimeVO& rhs)
        {
            return std::tie(lhs.m_timeSpanInSec, lhs.m_percent) ==
                   std::tie(rhs.m_timeSpanInSec, rhs.m_percent);
        }

        template <>
        bool less(
            _In_ const KeyTimeVO& lhs,
            _In_ const KeyTimeVO& rhs)
        {
            return std::tie(lhs.m_timeSpanInSec, lhs.m_percent) <
                   std::tie(rhs.m_timeSpanInSec, rhs.m_percent);
        }

        template <>
        std::size_t hash(
            _In_ const KeyTimeVO& inst)
        {
            std::size_t hash = 0;
            CommonUtilities::hash_combine(hash, inst.GetTimeSpanInSec());
            CommonUtilities::hash_combine(hash, inst.GetPercent());
            return hash;
        }
    }
}

bool operator==(
    _In_ const KeyTimeVO& lhs,
    _In_ const KeyTimeVO& rhs)
{
    return Flyweight::Operators::equal(lhs, rhs);
}

namespace KeyTimeVOHelper
{
    xref_ptr<KeyTimeVO::Wrapper> Create(
        _In_opt_ CCoreServices* core,
        double timeSpanInSec,
        float percent)
    {
        return Flyweight::Create<KeyTimeVO>(
            core,
            timeSpanInSec,
            percent);
    }

    xref_ptr<KeyTimeVO::Wrapper> CreateTimeSpan(
        _In_opt_ CCoreServices* core,
        double timeSpanInSec)
    {
        return Flyweight::Create<KeyTimeVO>(
            core,
            timeSpanInSec,
            KeyTimeVO::s_defaultPercent);
    }
}