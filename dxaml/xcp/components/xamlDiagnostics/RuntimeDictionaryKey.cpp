// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeDictionaryKey.h"
#include "RuntimeObject.h"
#include "RuntimeDictionary.h"

namespace Diagnostics
{
   RuntimeDictionaryKey::RuntimeDictionaryKey(const std::shared_ptr<RuntimeObject>& key)
        : m_key(key)
    {
        m_data.Key = m_key->ToString();
        // VS will either use the implicit style itself as the key, or the typename
        m_data.IsImplicitStyle = !m_key->IsValueType() || m_key->IsValueType<wxaml_interop::TypeName>();
    }

   RuntimeDictionaryKey::RuntimeDictionaryKey(const std::shared_ptr<RuntimeObject>& key, const std::shared_ptr<RuntimeDictionary>& owner)
        : RuntimeDictionaryKey(key)
    {
        // Only manage the lifetime of the key if the key is a ValueType. If VS uses the style itself as the key, then
        // the lifetime is already managed by RuntimeDictionaryValue
        if (key->IsValueType())
        {
            key->SetParent(owner);
        }
    }

    RuntimeDictionaryKey::RuntimeDictionaryKey(const RuntimeDictionaryKeyData& data)
        : m_data(data)
    {
    }

    bool RuntimeDictionaryKey::operator<(const RuntimeDictionaryKey& rhs) const
    {
        int32_t compareResult = GetKey().Compare(rhs.GetKey());
        if (compareResult == 0)
        {
            // The key strings match, check whether the key represents an ImplicitStyle.
            // It's possible for someone to have an implicit style of TargetType
            // "Microsoft.UI.Xaml.Controls.Button" as well as a resource with the x:Key "Microsoft.UI.Xaml.Controls.Button"
            return IsImplicitStyle() < rhs.IsImplicitStyle();
        }
        return compareResult < 0;
    }

    bool operator<(const RuntimeDictionaryKey& lhs, const std::shared_ptr<RuntimeObject>& rhs)
    {
        RuntimeDictionaryKey rhsKey(rhs);
        return lhs < rhsKey;
    }

    bool operator<(const std::shared_ptr<RuntimeObject>& lhs, const RuntimeDictionaryKey& rhs)
    {
        RuntimeDictionaryKey lhsKey(lhs);
        return lhsKey < rhs;
    }

    bool operator<(const RuntimeDictionaryKey& lhs, const RuntimeDictionaryKeyData& rhs)
    {
        RuntimeDictionaryKey rhsKey(rhs);
        return lhs < rhsKey;
    }

    bool operator<(const RuntimeDictionaryKeyData& lhs, const RuntimeDictionaryKey& rhs)
    {
        RuntimeDictionaryKey lhsKey(lhs);
        return lhsKey < rhs;
    }

    xstring_ptr RuntimeDictionaryKey::GetKey() const
    {
        return m_data.Key;
    }

    bool RuntimeDictionaryKey::IsImplicitStyle() const
    {
        return m_data.IsImplicitStyle;
    }
}