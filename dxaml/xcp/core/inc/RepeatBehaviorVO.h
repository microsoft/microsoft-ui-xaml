// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ValueObjectBase.h"
#include "Enumdefs.g.h"
#include "Indexes.g.h"
#include <xref_ptr.h>

class RepeatBehaviorVO
{
    // std::tie needs to work directly with member fields.
    friend bool Flyweight::Operators::equal<RepeatBehaviorVO>(const RepeatBehaviorVO&, const RepeatBehaviorVO&);
    friend bool Flyweight::Operators::less<RepeatBehaviorVO>(const RepeatBehaviorVO&, const RepeatBehaviorVO&);

public:
    using Wrapper = Flyweight::PropertyValueObjectWrapper<RepeatBehaviorVO>;

    static constexpr KnownTypeIndex                 s_typeIndex                 = KnownTypeIndex::RepeatBehavior;
    static constexpr double                         s_defaultDurationInSec      = 0.0;
    static constexpr DirectUI::RepeatBehaviorType   s_defaultRepeatBehaviorType = DirectUI::RepeatBehaviorType::Count;
    static constexpr float                          s_defaultCount              = 1.0f;

    RepeatBehaviorVO() = delete;

    RepeatBehaviorVO(
        double durationInSec,
        DirectUI::RepeatBehaviorType type,
        float count)
        : m_durationInSec(durationInSec)
        , m_type(type)
        , m_count(count)
    {}

    double GetDurationInSec() const
    {
        return m_durationInSec;
    }

    DirectUI::RepeatBehaviorType GetRepeatBehaviorType() const
    {
        return m_type;
    }

    float GetCount() const
    {
        return m_count;
    }

private:
    double m_durationInSec;
    DirectUI::RepeatBehaviorType m_type;
    float m_count;
};

namespace Flyweight
{
    namespace Operators
    {
        template <>
        RepeatBehaviorVO Default();

        template <>
        bool equal(
            _In_ const RepeatBehaviorVO& lhs,
            _In_ const RepeatBehaviorVO& rhs);

        template <>
        bool less(
            _In_ const RepeatBehaviorVO& lhs,
            _In_ const RepeatBehaviorVO& rhs);

        template <>
        std::size_t hash(
            _In_ const RepeatBehaviorVO& inst);
    }
}

bool operator==(
    _In_ const RepeatBehaviorVO& lhs,
    _In_ const RepeatBehaviorVO& rhs);

class CCoreServices;

namespace RepeatBehaviorVOHelper
{
    bool IsZeroLength(
        _In_ const RepeatBehaviorVO& value);

    double GetTotalDuration(
        const RepeatBehaviorVO& value,
        double iterationDuration,
        _Out_ bool* pLoopsForever);

    bool RepeatsOnce(
        _In_ const RepeatBehaviorVO& value);

    bool RepeatsForever(
        _In_ const RepeatBehaviorVO& value);

    float GetRepeatCount(
        _In_ const RepeatBehaviorVO& value,
        float naturalDuration);

    // Use these helpers to create flyweighted objects.
    // This will ensure only valid combinations of values are instantiated and keep number of elements to minimum.

    xref_ptr<RepeatBehaviorVO::Wrapper> Create(
        _In_opt_ CCoreServices* core,
        DirectUI::RepeatBehaviorType type = RepeatBehaviorVO::s_defaultRepeatBehaviorType,
        double durationInSec = RepeatBehaviorVO::s_defaultDurationInSec,
        float count = RepeatBehaviorVO::s_defaultCount);

    xref_ptr<RepeatBehaviorVO::Wrapper> CreateCount(
        _In_opt_ CCoreServices* core,
        float count = RepeatBehaviorVO::s_defaultCount);

    xref_ptr<RepeatBehaviorVO::Wrapper> CreateDuration(
        _In_opt_ CCoreServices* core,
        double durationInSec = RepeatBehaviorVO::s_defaultDurationInSec);

    xref_ptr<RepeatBehaviorVO::Wrapper> CreateForever(
        _In_opt_ CCoreServices* core);
}