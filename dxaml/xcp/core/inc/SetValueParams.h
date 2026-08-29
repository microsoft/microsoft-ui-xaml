// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BaseValueSource.h"

class CDependencyProperty;
class CValue;
class CModifiedValue;

struct SetValueParams
{
    const CDependencyProperty* m_pDP;
    const CValue& m_value;
    const std::shared_ptr<CModifiedValue>& m_modifierValueBeingSet;
    ::BaseValueSource m_baseValueSource;
    IInspectable* m_pValueOuterNoRef;

    SetValueParams(
        _In_ const CDependencyProperty* dp,
        _In_ const CValue& value,
        _In_::BaseValueSource baseValueSource = BaseValueSourceUnknown,
        _In_opt_ IInspectable* valueOuterNoRef = nullptr,
        _In_ const std::shared_ptr<CModifiedValue>& modifierValueBeingSet = std::shared_ptr<CModifiedValue>())
        : m_pDP(dp),
          m_value(value),
          m_baseValueSource(baseValueSource),
          m_pValueOuterNoRef(valueOuterNoRef),
          m_modifierValueBeingSet(modifierValueBeingSet)
    {}

    SetValueParams(
        _In_ const SetValueParams& args,
        _In_ const CValue& value)
        : m_pDP(args.m_pDP),
          m_value(value),
          m_baseValueSource(args.m_baseValueSource),
          m_pValueOuterNoRef(args.m_pValueOuterNoRef),
          m_modifierValueBeingSet(args.m_modifierValueBeingSet)
    {}

    SetValueParams& operator=(const SetValueParams& other) = delete;
};
