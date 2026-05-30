// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ValueObjectBase.h"
#include "Enumdefs.g.h"
#include "Indexes.g.h"
#include <xref_ptr.h>

class DurationVO
{
    // std::tie needs to work directly with member fields.
    friend bool Flyweight::Operators::equal<DurationVO>(const DurationVO&, const DurationVO&);
    friend bool Flyweight::Operators::less<DurationVO>(const DurationVO&, const DurationVO&);

public:
    using Wrapper = Flyweight::PropertyValueObjectWrapper<DurationVO>;

    static constexpr KnownTypeIndex         s_typeIndex             = KnownTypeIndex::Duration;
    static constexpr double                 s_defaultTimeSpanInSec  = 0.0;
    static constexpr DirectUI::DurationType s_defaultDurationType   = DirectUI::DurationType::Automatic;

    DurationVO() = delete;

    DurationVO(
        double timeSpanInSec,
        DirectUI::DurationType type)
        : m_timeSpanInSec(timeSpanInSec)
        , m_type(type)
    {}

    double GetTimeSpanInSec() const
    {
        return m_timeSpanInSec;
    }

    DirectUI::DurationType GetDurationType() const
    {
        return m_type;
    }

private:
    double m_timeSpanInSec;
    DirectUI::DurationType m_type;
};

namespace Flyweight
{
    namespace Operators
    {
        template <>
        DurationVO Default();

        template <>
        bool equal(
            _In_ const DurationVO& lhs,
            _In_ const DurationVO& rhs);

        template <>
        bool less(
            _In_ const DurationVO& lhs,
            _In_ const DurationVO& rhs);

        template <>
        std::size_t hash(
            _In_ const DurationVO& inst);
    }
}

bool operator==(
    _In_ const DurationVO& lhs,
    _In_ const DurationVO& rhs);

class CCoreServices;

namespace DurationVOHelper
{
    bool IsZeroLength(
        _In_ const DurationVO& value);

    // Use these helpers to create flyweighted objects.
    // This will ensure only valid combinations of values are instantiated and keep number of elements to minimum.

    xref_ptr<DurationVO::Wrapper> Create(
        _In_opt_ CCoreServices* core,
        DirectUI::DurationType type = DurationVO::s_defaultDurationType,
        double timeSpanInSec = DurationVO::s_defaultTimeSpanInSec);

    xref_ptr<DurationVO::Wrapper> CreateTimeSpan(
        _In_opt_ CCoreServices* core,
        double timeSpanInSec = DurationVO::s_defaultTimeSpanInSec);

    xref_ptr<DurationVO::Wrapper> CreateAutomatic(
        _In_opt_ CCoreServices* core);

    xref_ptr<DurationVO::Wrapper> CreateForever(
        _In_opt_ CCoreServices* core);
}