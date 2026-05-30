// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
// This file will be code-generated.
namespace SimpleProperty
{
    // Specializations for host types.

    template <>
    struct sparse_host<SomeClass>
    {
        using declaring_type = SomeClass;
        static constexpr sparse_host_query_t<declaring_type> query = &declaring_type::IsSimpleSparseSet;
        static constexpr sparse_host_setter_t<declaring_type> setter = &declaring_type::SetSimpleSparseFlag;
    };

    namespace details
    {
        // Sparse tables

        template <>
        static auto& PickSparseTable<PID::valueInSparse>(sparsetables& tables)
        {
            return tables.m_valueInSparse;
        }

        template <>
        static auto& PickSparseTable<PID::smartInSparse>(sparsetables& tables)
        {
            return tables.m_smartInSparse;
        }

        template <>
        static auto& PickSparseTable<PID::bigImmutableInSparse>(sparsetables& tables)
        {
            return tables.m_bigImmutableInSparse;
        }

        template <>
        static auto& PickSparseTable<PID::bigMutableInSparse>(sparsetables& tables)
        {
            return tables.m_bigMutableInSparse;
        }

        template <>
        static auto& PickSparseTable<PID::valueInGroupSparse>(sparsetables& tables)
        {
            return tables.m_groupStorage;
        }

        template <>
        static auto& PickSparseTable<PID::smartInGroupSparse>(sparsetables& tables)
        {
            return tables.m_groupStorage;
        }

        template <>
        static auto& PickSparseTable<PID::bigImmutableInGroupSparse>(sparsetables& tables)
        {
            return tables.m_groupStorage;
        }

        template <>
        static auto& PickSparseTable<PID::bigMutableInGroupSparse>(sparsetables& tables)
        {
            return tables.m_groupStorage;
        }

        // Change handlers

        template <>
        static auto& PickHandlersTable<PID::valueInSparse>(changehandlerstables& handlers)
        {
            return handlers.m_valueInSparse;
        }

        template <>
        static auto& PickHandlersTable<PID::valueInField>(changehandlerstables& handlers)
        {
            return handlers.m_valueInField;
        }

        template <>
        static auto& PickHandlersTable<PID::valueInGroupSparse>(changehandlerstables& handlers)
        {
            return handlers.m_valueInGroupSparse;
        }

        template <>
        static auto& PickHandlersTable<PID::smartInSparse>(changehandlerstables& handlers)
        {
            return handlers.m_smartInSparse;
        }

        template <>
        static auto& PickHandlersTable<PID::smartInField>(changehandlerstables& handlers)
        {
            return handlers.m_smartInField;
        }

        template <>
        static auto& PickHandlersTable<PID::smartInGroupSparse>(changehandlerstables& handlers)
        {
            return handlers.m_smartInGroupSparse;
        }

        template <>
        static auto& PickHandlersTable<PID::bigImmutableInSparse>(changehandlerstables& handlers)
        {
            return handlers.m_bigImmutableInSparse;
        }

        template <>
        static auto& PickHandlersTable<PID::bigImmutableInField>(changehandlerstables& handlers)
        {
            return handlers.m_bigImmutableInField;
        }

        template <>
        static auto& PickHandlersTable<PID::bigImmutableInGroupSparse>(changehandlerstables& handlers)
        {
            return handlers.m_bigImmutableInGroupSparse;
        }

        template <>
        static auto& PickHandlersTable<PID::bigMutableInSparse>(changehandlerstables& handlers)
        {
            return handlers.m_bigMutableInSparse;
        }

        template <>
        static auto& PickHandlersTable<PID::bigMutableInField>(changehandlerstables& handlers)
        {
            return handlers.m_bigMutableInField;
        }

        template <>
        static auto& PickHandlersTable<PID::bigMutableInGroupSparse>(changehandlerstables& handlers)
        {
            return handlers.m_bigMutableInGroupSparse;
        }

        const RuntimePropertyInfo s_runtimePropertyInfos[] =
        {
            GENERATE_RUNTIMEINFO(PID::valueInSparse),
            GENERATE_RUNTIMEINFO(PID::valueInField),
            GENERATE_RUNTIMEINFO(PID::valueInGroupSparse),
            GENERATE_RUNTIMEINFO(PID::smartInSparse),
            GENERATE_RUNTIMEINFO(PID::smartInField),
            GENERATE_RUNTIMEINFO(PID::smartInGroupSparse),
            GENERATE_RUNTIMEINFO(PID::bigImmutableInSparse),
            GENERATE_RUNTIMEINFO(PID::bigImmutableInField),
            GENERATE_RUNTIMEINFO(PID::bigImmutableInGroupSparse),
            GENERATE_RUNTIMEINFO(PID::bigMutableInSparse),
            GENERATE_RUNTIMEINFO(PID::bigMutableInField),
            GENERATE_RUNTIMEINFO(PID::bigMutableInGroupSparse)
        };
    }

    template <>
    void Property::NotifyDestroyed<SomeClass>(objid_t obj)
    {
        id<PID::valueInSparse>::DestroyInstance(obj);
        id<PID::valueInField>::DestroyInstance(obj);
        id<PID::valueInGroupSparse>::DestroyInstance(obj);
        id<PID::smartInSparse>::DestroyInstance(obj);
        id<PID::smartInField>::DestroyInstance(obj);
        id<PID::smartInGroupSparse>::DestroyInstance(obj);
        id<PID::bigImmutableInSparse>::DestroyInstance(obj);
        id<PID::bigImmutableInField>::DestroyInstance(obj);
        id<PID::bigImmutableInGroupSparse>::DestroyInstance(obj);
        id<PID::bigMutableInSparse>::DestroyInstance(obj);
        id<PID::bigMutableInField>::DestroyInstance(obj);
        id<PID::bigMutableInGroupSparse>::DestroyInstance(obj);
    }

    void Property::DestroyAllProperties()
    {
        id<PID::valueInSparse>::DestroyProperty();
        id<PID::valueInField>::DestroyProperty();
        id<PID::valueInGroupSparse>::DestroyProperty();
        id<PID::smartInSparse>::DestroyProperty();
        id<PID::smartInField>::DestroyProperty();
        id<PID::smartInGroupSparse>::DestroyProperty();
        id<PID::bigImmutableInSparse>::DestroyProperty();
        id<PID::bigImmutableInField>::DestroyProperty();
        id<PID::bigImmutableInGroupSparse>::DestroyProperty();
        id<PID::bigMutableInSparse>::DestroyProperty();
        id<PID::bigMutableInField>::DestroyProperty();
        id<PID::bigMutableInGroupSparse>::DestroyProperty();
    }

    template class Property::id<PID::valueInSparse>;
    template class Property::id<PID::valueInField>;
    template class Property::id<PID::valueInGroupSparse>;
    template class Property::id<PID::smartInSparse>;
    template class Property::id<PID::smartInField>;
    template class Property::id<PID::smartInGroupSparse>;
    template class Property::id<PID::bigImmutableInSparse>;
    template class Property::id<PID::bigImmutableInField>;
    template class Property::id<PID::bigImmutableInGroupSparse>;
    template class Property::id<PID::bigMutableInSparse>;
    template class Property::id<PID::bigMutableInField>;
    template class Property::id<PID::bigMutableInGroupSparse>;
}