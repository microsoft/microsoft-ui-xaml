// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeDictionaryValue.h"
#include "RuntimeObject.h"
#include "RuntimeDictionary.h"

namespace Diagnostics
{
    RuntimeDictionaryValue::RuntimeDictionaryValue(const std::shared_ptr<RuntimeObject>& value, const std::shared_ptr<RuntimeDictionary>& owner)
        : m_value(value)
    {
        if (m_value)
        {
            // The lifetime of the value is owned by the RuntimeDictionary
            m_value->SetParent(owner);
        }
    }

    RuntimeDictionaryValue::~RuntimeDictionaryValue()
    {
        // Notify the current value (if exists) that it no longer has a parent.
        if (m_value)
        {
            m_value->SetParent(nullptr);
        }
    }

    std::shared_ptr<RuntimeObject> RuntimeDictionaryValue::GetValue() const
    {
        return m_value;
    }
}