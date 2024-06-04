// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ValueObjectBase.h"
#include "Indexes.g.h"
#include <xref_ptr.h>

class KeyTimeVO
{
    // std::tie needs to work directly with member fields.
    friend bool Flyweight::Operators::equal<KeyTimeVO>(const KeyTimeVO&, const KeyTimeVO&);
    friend bool Flyweight::Operators::less<KeyTimeVO>(const KeyTimeVO&, const KeyTimeVO&);

public:
    using Wrapper = Flyweight::PropertyValueObjectWrapper<KeyTimeVO>;

    static constexpr KnownTypeIndex s_typeIndex             = KnownTypeIndex::KeyTime;
    static constexpr double         s_defaultTimeSpanInSec  = 0.0;
    static constexpr float          s_defaultPercent        = 0.0f;

    KeyTimeVO() = delete;

    KeyTimeVO(
        double timeSpanInSec,
        float percent)
        : m_timeSpanInSec(timeSpanInSec)
        , m_percent(percent)
    {}

    double GetTimeSpanInSec() const
    {
        return m_timeSpanInSec;
    }

    float GetPercent() const
    {
        return m_percent;
    }

private:
    double m_timeSpanInSec;
    float m_percent;
};

namespace Flyweight
{
    namespace Operators
    {
        template <>
        KeyTimeVO Default();

        template <>
        bool equal(
            _In_ const KeyTimeVO& lhs,
            _In_ const KeyTimeVO& rhs);

        template <>
        bool less(
            _In_ const KeyTimeVO& lhs,
            _In_ const KeyTimeVO& rhs);

        template <>
        std::size_t hash(
            _In_ const KeyTimeVO& inst);
    }
}

bool operator==(
    _In_ const KeyTimeVO& lhs,
    _In_ const KeyTimeVO& rhs);

class CCoreServices;

namespace KeyTimeVOHelper
{
    // Use these helpers to create flyweighted objects.
    // This will ensure only valid combinations of values are instantiated and keep number of elements to minimum.

    xref_ptr<KeyTimeVO::Wrapper> Create(
        _In_opt_ CCoreServices* core,
        double timeSpanInSec = KeyTimeVO::s_defaultTimeSpanInSec,
        float percent = KeyTimeVO::s_defaultPercent);

    xref_ptr<KeyTimeVO::Wrapper> CreateTimeSpan(
        _In_opt_ CCoreServices* core,
        double timeSpanInSec = KeyTimeVO::s_defaultTimeSpanInSec);
}