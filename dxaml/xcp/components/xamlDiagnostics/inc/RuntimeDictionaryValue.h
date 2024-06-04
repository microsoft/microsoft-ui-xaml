// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Diagnostics
{
    class RuntimeObject;
    class RuntimeDictionary;

    class RuntimeDictionaryValue final
    {
    public:
        RuntimeDictionaryValue(const std::shared_ptr<RuntimeObject>& value, const std::shared_ptr<RuntimeDictionary>& owner);
        ~RuntimeDictionaryValue();
        std::shared_ptr<RuntimeObject> GetValue() const;
        
        // Prevent copying to avoid incorrect unparenting.
        RuntimeDictionaryValue(const RuntimeDictionaryValue& rhs) = delete;
        RuntimeDictionaryValue& operator=(const RuntimeDictionaryValue& rhs) = delete;
        RuntimeDictionaryValue(RuntimeDictionaryValue&& rhs) = default;
        RuntimeDictionaryValue& operator=(RuntimeDictionaryValue&& rhs) = default;
    private:
        std::shared_ptr<RuntimeObject> m_value;
    };
}