// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <xstring_ptr.h>

namespace Diagnostics
{
    class RuntimeObject;
    class RuntimeDictionary;

    struct RuntimeDictionaryKeyData
    {
        bool IsImplicitStyle = false;
        xstring_ptr Key;
    };

    class RuntimeDictionaryKey final
    {
    public:
        RuntimeDictionaryKey(const std::shared_ptr<RuntimeObject>& key, const std::shared_ptr<RuntimeDictionary>& owner);
        RuntimeDictionaryKey(const RuntimeDictionaryKeyData& data);

        bool operator<(const RuntimeDictionaryKey& rhs) const;
        RuntimeDictionaryKey(const std::shared_ptr<RuntimeObject>& key);
    private:
        xstring_ptr GetKey() const;
        bool IsImplicitStyle() const;
    private:
        std::shared_ptr<RuntimeObject> m_key;
        RuntimeDictionaryKeyData m_data;
    };

    bool operator<(const RuntimeDictionaryKey& lhs, const std::shared_ptr<RuntimeObject>& rhs);
    bool operator<(const std::shared_ptr<RuntimeObject>& lhs, const RuntimeDictionaryKey& rhs);
    bool operator<(const RuntimeDictionaryKey& lhs, const RuntimeDictionaryKeyData& rhs);
    bool operator<(const RuntimeDictionaryKeyData& lhs, const RuntimeDictionaryKey& rhs);
}