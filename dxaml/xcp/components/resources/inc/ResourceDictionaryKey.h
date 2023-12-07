// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xstring_ptr.h>

namespace ResourceDictionaryKey
{
    namespace Compare
    {
        template <typename T, typename U> bool equals(const T& lhs, const U& rhs);
    }

    namespace details
    {
        constexpr std::size_t c_isTypeKeyMask   = 1;
        constexpr std::size_t c_hashMask        = ~c_isTypeKeyMask;

        inline std::size_t EncodeHashAndIsTypeHelper(std::size_t hashAndIsType, bool keyIsType)
        {
            return (hashAndIsType & c_hashMask) | ((keyIsType) ? c_isTypeKeyMask : 0);
        }

        inline std::size_t EncodeHashAndIsType(const xstring_ptr_view& key, bool keyIsType)
        {
            return EncodeHashAndIsTypeHelper(key.GetHash(), keyIsType);
        }
    }
}

struct ResourceKey;

// ResourceKeyStorage is meant to be used in places where key is non-transient.

struct ResourceKeyStorage
{
    friend struct ResourceKey;
    template <typename T, typename U> friend bool ResourceDictionaryKey::Compare::equals(const T& lhs, const U& rhs);

    ResourceKeyStorage() = default;

    ResourceKeyStorage(const ResourceKeyStorage&) = default;

    ResourceKeyStorage(ResourceKeyStorage&&) noexcept = default;

    explicit ResourceKeyStorage(const xstring_ptr& key, bool keyIsType)
        : m_key(key)
        , m_hashAndIsKeyType(ResourceDictionaryKey::details::EncodeHashAndIsType(key, keyIsType))
    {}

    ResourceKeyStorage& operator=(const ResourceKeyStorage& rhs) = default;

    ResourceKeyStorage& operator=(ResourceKeyStorage&& rhs) noexcept = default;

    const xstring_ptr& GetKey() const
    {
        return m_key;
    }

    std::size_t hash() const
    {
        return m_hashAndIsKeyType & ResourceDictionaryKey::details::c_hashMask;
    }

    bool IsKeyType() const
    {
        return m_hashAndIsKeyType & ResourceDictionaryKey::details::c_isTypeKeyMask;
    }

private:
    explicit ResourceKeyStorage(const xstring_ptr& key, std::size_t hashAndIsKeyType)
        : m_key(key)
        , m_hashAndIsKeyType(hashAndIsKeyType)
    {}

    xstring_ptr m_key;

    // To avoid spilling over 2 pointers, encode key-is-type as LSB of the hash.
    std::size_t m_hashAndIsKeyType = 0;
};

// ResourceKey is meant to be used in queries where key is transient (e.g. lives on stack).

struct ResourceKey
{
    template <typename T, typename U> friend bool ResourceDictionaryKey::Compare::equals(const T& lhs, const U& rhs);

    explicit ResourceKey(const xstring_ptr_view& key, bool keyIsType)
        : m_key(key)
        , m_hashAndIsKeyType(ResourceDictionaryKey::details::EncodeHashAndIsType(key, keyIsType))
    {}

    // Constructor to reuse key hash and modify key-is-type.

    explicit ResourceKey(const ResourceKey& other, bool keyIsType)
        : m_key(other.GetKey())
        , m_hashAndIsKeyType(ResourceDictionaryKey::details::EncodeHashAndIsTypeHelper(other.hash(), keyIsType))
        , m_shouldFilter(other.ShouldFilter())
    {}

    const xstring_ptr_view& GetKey() const
    {
        return m_key;
    }

    std::size_t hash() const
    {
        return m_hashAndIsKeyType & ResourceDictionaryKey::details::c_hashMask;
    }

    bool IsKeyType() const
    {
        return m_hashAndIsKeyType & ResourceDictionaryKey::details::c_isTypeKeyMask;
    }

    ResourceKeyStorage ToStorage() const
    {
        xstring_ptr key;
        IFCFAILFAST(m_key.Promote(&key));
        return ResourceKeyStorage(key, m_hashAndIsKeyType);
    }

    void ClearShouldFilter()
    {
        m_shouldFilter = false;
    }

    bool ShouldFilter() const
    {
        return m_shouldFilter;
    }

private:
    const xstring_ptr_view& m_key;
    std::size_t m_hashAndIsKeyType;
    bool m_shouldFilter = true;
};

namespace std
{
    template <> struct hash<ResourceKey>
    {
        std::size_t operator()(const ResourceKey& value) const
        {
            return value.hash();
        }
    };

    template <> struct hash<ResourceKeyStorage>
    {
        std::size_t operator()(const ResourceKeyStorage& value) const
        {
            return value.hash();
        }
    };
}

namespace ResourceDictionaryKey
{
    namespace Compare
    {
        template <typename T, typename U>
        bool equals(const T& lhs, const U& rhs)
        {
            return std::tie(lhs.m_hashAndIsKeyType, lhs.m_key) == std::tie(rhs.m_hashAndIsKeyType, rhs.m_key);
        }
    }
}

inline bool operator==(const ResourceKeyStorage& lhs, const ResourceKeyStorage& rhs)
{
    return ResourceDictionaryKey::Compare::equals(lhs, rhs);
}

inline bool operator==(const ResourceKey& lhs, const ResourceKeyStorage& rhs)
{
    return ResourceDictionaryKey::Compare::equals(lhs, rhs);
}

inline bool operator==(const ResourceKeyStorage& lhs, const ResourceKey& rhs)
{
    return ResourceDictionaryKey::Compare::equals(lhs, rhs);
}