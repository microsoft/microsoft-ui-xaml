// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xstring_ptr.h>
#include <cstdint>

namespace ResourceDictionaryKey
{
    namespace Compare
    {
        template <typename T, typename U> bool equals(const T& lhs, const U& rhs);
    }

    namespace details
    {
        constexpr std::uint64_t c_isTypeKeyMask   = 1;
        constexpr std::uint64_t c_hashMask        = ~c_isTypeKeyMask;

        inline std::uint64_t EncodeHashAndIsTypeHelper(std::uint64_t hashAndIsType, bool keyIsType)
        {
            return (hashAndIsType & c_hashMask) | ((keyIsType) ? c_isTypeKeyMask : 0);
        }

        inline std::uint64_t EncodeHashAndIsType(const xstring_ptr_view& key, bool keyIsType)
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

    // Tag type for constructing a ResourceKeyStorage without computing a hash.
    // Used in containment fallback paths that only need string-based Equals comparison.
    struct NoHash {};

    explicit ResourceKeyStorage(const xstring_ptr& key, NoHash)
        : m_key(key)
        , m_hashAndIsKeyType(0)
#if DBG
        , m_noHash(true)
#endif
    {}

    ResourceKeyStorage& operator=(const ResourceKeyStorage& rhs) = default;

    ResourceKeyStorage& operator=(ResourceKeyStorage&& rhs) noexcept = default;

    const xstring_ptr& GetKey() const
    {
        return m_key;
    }

    std::uint64_t hash() const
    {
#if DBG
        ASSERT(!m_noHash);
#endif
        return m_hashAndIsKeyType & ResourceDictionaryKey::details::c_hashMask;
    }

    // Style resources that do not specify an x:Key/x:Name are internally known as "implicit resources"
    // since they do not have an explicitly specified key. Instead, they use the fully-qualified name
    // of the TargetType as their key in the internal associative map. This method returns `true`
    // if this `ResourceKeyStorage` is associated with an implicit resource.
    bool IsKeyType() const
    {
#if DBG
        ASSERT(!m_noHash);
#endif
        return m_hashAndIsKeyType & ResourceDictionaryKey::details::c_isTypeKeyMask;
    }

    // Creates a transient ResourceKey that references this storage's key
    // and reuses its pre-computed hash.  The returned ResourceKey is only
    // valid while this ResourceKeyStorage is alive.
    inline ResourceKey ToResourceKey() const;

private:
    explicit ResourceKeyStorage(const xstring_ptr& key, std::uint64_t precomputedHashAndIsKeyType)
        : m_key(key)
        , m_hashAndIsKeyType(precomputedHashAndIsKeyType)
    {
#if DBG
        // Assuming that the precomputed IsKeyType is correct, compare the precomputed hash against the
        // live hash and verify that they match.
        bool keyIsType = precomputedHashAndIsKeyType & ResourceDictionaryKey::details::c_isTypeKeyMask;
        auto liveHashAndIsKeyType = ResourceDictionaryKey::details::EncodeHashAndIsType(key, keyIsType);

        if (liveHashAndIsKeyType != precomputedHashAndIsKeyType)
        {
            // Precomputed hash does not match live hash indicating that the hash algorithm has changed.
            // Investigate root cause and mitigate.
            IFCFAILFAST(E_UNEXPECTED);
        }
#endif
    }

    xstring_ptr m_key;

    // To avoid spilling over 2 pointers, encode key-is-type as LSB of the hash.
    std::uint64_t m_hashAndIsKeyType = 0;
#if DBG
    // When m_noHash is true, the m_hashAndIsKeyType field should not be used.
    // This is only for the legacy containment path.
    bool m_noHash = false; 
#endif
};

// ResourceKey is meant to be used in queries where key is transient (e.g. lives on stack).

struct ResourceKey
{
    template <typename T, typename U> friend bool ResourceDictionaryKey::Compare::equals(const T& lhs, const U& rhs);

    // Tag type for constructing a ResourceKey with a pre-computed hash,
    // skipping the hash computation on the key string.
    struct PrecomputedHash {};

    // Tag type for constructing a ResourceKey without computing a hash.
    // Used in containment fallback paths that only need string-based Equals comparison.
    struct NoHash {};

    explicit ResourceKey(const xstring_ptr_view& key, bool keyIsType)
        : m_key(key)
        , m_hashAndIsKeyType(ResourceDictionaryKey::details::EncodeHashAndIsType(key, keyIsType))
    {}

    explicit ResourceKey(const xstring_ptr_view& key, std::uint64_t precomputedHashAndIsKeyType, PrecomputedHash)
        : m_key(key)
        , m_hashAndIsKeyType(precomputedHashAndIsKeyType)
    {
#if DBG
        // Assuming that the precomputed IsKeyType is correct, compare the precomputed hash against the
        // live hash and verify that they match.
        bool keyIsType = precomputedHashAndIsKeyType & ResourceDictionaryKey::details::c_isTypeKeyMask;
        auto liveHashAndIsKeyType = ResourceDictionaryKey::details::EncodeHashAndIsType(key, keyIsType);

        if (liveHashAndIsKeyType != precomputedHashAndIsKeyType)
        {
            // Precomputed hash does not match live hash indicating that the hash algorithm has changed.
            // Investigate root cause and mitigate.
            IFCFAILFAST(E_UNEXPECTED);
        }
#endif
    }

    // Constructor to reuse key hash and modify key-is-type.

    explicit ResourceKey(const ResourceKey& other, bool keyIsType)
        : m_key(other.GetKey())
        , m_hashAndIsKeyType(ResourceDictionaryKey::details::EncodeHashAndIsTypeHelper(other.hash(), keyIsType))
        , m_shouldFilter(other.ShouldFilter())
    {}

    // Construct without computing a hash. Only for use in containment fallback paths
    // that compare via string Equals rather than hash-based operator==.
    explicit ResourceKey(const xstring_ptr_view& key, NoHash)
        : m_key(key)
        , m_hashAndIsKeyType(0)
#if DBG
        , m_noHash(true)
#endif
    {}

    ResourceKey& operator=(const ResourceKey& other) noexcept
    {
        if (this != &other)
        {
            this->~ResourceKey();
            auto p = ::new (this) ResourceKey(other);
            return *p;
        }
        return *this;
    }

    const xstring_ptr_view& GetKey() const
    {
        return m_key;
    }

    std::uint64_t hash() const
    {
#if DBG
        ASSERT(!m_noHash);
#endif
        return m_hashAndIsKeyType & ResourceDictionaryKey::details::c_hashMask;
    }

    // Style resources that do not specify an x:Key/x:Name are internally known as "implicit resources"
    // since they do not have an explicitly specified key. Instead, they use the fully-qualified name
    // of the TargetType as their key in the internal associative map. This method returns `true`
    // if this `ResourceKey` is associated with an implicit resource.
    bool IsKeyType() const
    {
#if DBG
        ASSERT(!m_noHash);
#endif
        return m_hashAndIsKeyType & ResourceDictionaryKey::details::c_isTypeKeyMask;
    }

    ResourceKeyStorage ToStorage()const
    {
#if DBG
        ASSERT(!m_noHash);
#endif
        xstring_ptr key;
        IFCFAILFAST(m_key.Promote(&key));
        return ResourceKeyStorage(key, m_hashAndIsKeyType);
    }

    ResourceKeyStorage ToStorageNoHash() const
    {
        xstring_ptr key;
        IFCFAILFAST(m_key.Promote(&key));
        return ResourceKeyStorage(key, ResourceKeyStorage::NoHash{});
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
    std::uint64_t m_hashAndIsKeyType;
    bool m_shouldFilter = true;
#if DBG
    // When m_noHash is true, the m_hashAndIsKeyType field should not be used.
    // This is only for the legacy containment path.
    bool m_noHash = false; 
#endif
};

inline ResourceKey ResourceKeyStorage::ToResourceKey() const
{
#if DBG
    ASSERT(!m_noHash);
#endif
    return ResourceKey(m_key, m_hashAndIsKeyType, ResourceKey::PrecomputedHash{});
}

namespace std
{
    template <> struct hash<ResourceKey>
    {
        std::size_t operator()(const ResourceKey& value) const
        {
            return static_cast<std::size_t>(value.hash());
        }
    };

    template <> struct hash<ResourceKeyStorage>
    {
        std::size_t operator()(const ResourceKeyStorage& value) const
        {
            return static_cast<std::size_t>(value.hash());
        }
    };
}

// Transparent hash for ResourceKeyStorage / ResourceKey.
// Allows heterogeneous lookup: a transient ResourceKey can probe a map keyed
// by ResourceKeyStorage without promoting the xstring_ptr_view to xstring_ptr.
// The hash is pre-computed at key construction time and already incorporates
// the IsKeyType flag, so operator() just returns it.
struct ResourceKeyStorage_transparent_hash
{
    using is_transparent = void;
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const ResourceKeyStorage& key) const noexcept -> uint64_t
    {
        return key.hash();
    }

    [[nodiscard]] auto operator()(const ResourceKey& key) const noexcept -> uint64_t
    {
        return key.hash();
    }
};

// Transparent equality for ResourceKeyStorage / ResourceKey.
struct ResourceKeyStorage_transparent_equal
{
    using is_transparent = void;

    bool operator()(const ResourceKeyStorage& lhs, const ResourceKeyStorage& rhs) const
    {
        return ResourceDictionaryKey::Compare::equals(lhs, rhs);
    }

    bool operator()(const ResourceKey& lhs, const ResourceKeyStorage& rhs) const
    {
        return ResourceDictionaryKey::Compare::equals(lhs, rhs);
    }

    bool operator()(const ResourceKeyStorage& lhs, const ResourceKey& rhs) const
    {
        return ResourceDictionaryKey::Compare::equals(lhs, rhs);
    }
};

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

inline bool operator==(const ResourceKey& lhs, const ResourceKey& rhs)
{
    return ResourceDictionaryKey::Compare::equals(lhs, rhs);
}
