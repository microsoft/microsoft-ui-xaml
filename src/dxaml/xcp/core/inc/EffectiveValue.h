// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cvalue.h>

struct EffectiveValue
{
    EffectiveValue() = default;

    EffectiveValue(EffectiveValue&& other) noexcept
        : value(std::move(other.value))
    {}

    EffectiveValue& operator=(EffectiveValue&& other) noexcept
    {
        if (this != &other)
        {
            value = std::move(other.value);
        }
        return *this;
    }

    EffectiveValue(const EffectiveValue&) = delete;
    EffectiveValue& operator=(const EffectiveValue&) = delete;

    _Check_return_ HRESULT CopyFrom(const EffectiveValue& other)
    {
        return value.CopyConverted(other.value);
    }

    void SetIsSetLocally(bool isSetLocally)     { value.GetCustomData().SetIsSetLocally(isSetLocally); }
    bool IsSetLocally() const                   { return value.GetCustomData().IsSetLocally(); }

    void SetIsSetByStyle(bool isSetByStyle)     { value.GetCustomData().SetIsSetByStyle(isSetByStyle); }
    bool IsSetByStyle() const                   { return value.GetCustomData().IsSetByStyle(); }

    bool IsRawIInspectable() const              { return value.GetType() == valueIInspectable && !value.OwnsValue(); }

    CValue value;
};

static_assert(sizeof(EffectiveValue) <= sizeof(CValue), "Size of EffectiveValue should not be larger than size of CValue.");
static_assert(alignof(EffectiveValue) <= alignof(CValue), "Alignment of EffectiveValue should not be larger than size of CValue.");
