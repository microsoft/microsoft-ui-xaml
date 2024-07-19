// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <utility>
#include "StaticAssertFalse.h"

enum class KnownTypeIndex : UINT16;

namespace Flyweight
{
    template <typename T>
    class ValueObjectWrapper
    {
    public:
        using value_type = T;

        template <typename ...Types>
        ValueObjectWrapper(
            Types&&... args)
            : m_value(std::forward<Types>(args)...)
        {}

        const T& Value() const
        {
            return m_value;
        }

        void AddRef() const
        {
            ++m_refCount;
        }

        unsigned int Release() const
        {
            unsigned int refs = --m_refCount;

            if (refs == 0)
            {
                delete this;
            }

            return refs;
        }

        unsigned GetRefCount() const
        {
            return m_refCount;
        }

    private:
        const T m_value;
        mutable unsigned int m_refCount = 1;
    };

    class PropertyValueObjectBase
    {
    public:
        virtual ~PropertyValueObjectBase() {}

        virtual KnownTypeIndex GetTypeIndex() const = 0;

        void AddRef() const
        {
            ++m_refCount;
        }

        unsigned int Release() const
        {
            unsigned int refs = --m_refCount;

            if (refs == 0)
            {
                delete this;
            }

            return refs;
        }

        unsigned GetRefCount() const
        {
            return m_refCount;
        }

    private:
        mutable unsigned int m_refCount = 1;
    };

    template <typename T>
    class PropertyValueObjectWrapper : public PropertyValueObjectBase
    {
    public:
        using value_type = T;

        template <typename ...Types>
        PropertyValueObjectWrapper(
            _In_ Types&&... args)
            : m_value(std::forward<Types>(args)...)
        {}

        KnownTypeIndex GetTypeIndex() const override
        {
            return value_type::s_typeIndex;
        }

        const T& Value() const
        {
            return m_value;
        }

    private:
        const T m_value;
    };

    namespace Operators
    {
        template <typename T>
        T Default()
        {
            static_assert_false("Specialize Default() for your value object type.");
        }

        template <typename T>
        bool equal(_In_ const T&, _In_ const T&)
        {
            static_assert_false("Specialize equal() for your value object type.");
        }

        template <typename T>
        bool less(_In_ const T&, _In_ const T&)
        {
            static_assert_false("Specialize less() for your value object type.");
        }

        template <typename T>
        std::size_t hash(_In_ const T&)
        {
            static_assert_false("Specialize hash() for your value object type.");
        }
    }

    template <typename T>
    struct Wrapper
    {
        using Type = typename T::Wrapper;
    };
}