// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This file will be code-generated.
// Do not include directly, but rather via SimpleProperties.h, which takes care of ordering dependencies.

class SomeClass;

namespace SimpleProperty
{
    // Type specializations

    template <>
    struct type_traits<TID::Integer>
    {
        static constexpr TID tid = TID::Integer;
        static constexpr Kind kind = Kind::Value;
        using type = int;
    };

    template <>
    struct type_traits<TID::SharedBigStructOfInt>
    {
        static constexpr TID tid = TID::SharedBigStructOfInt;
        static constexpr Kind kind = Kind::ValueByRef;
        using type = std::shared_ptr<BigStruct<int>>;
    };

    template <>
    struct type_traits<TID::BigStructOfInt>
    {
        static constexpr TID tid = TID::BigStructOfInt;
        static constexpr Kind kind = Kind::BigImmutable;
        using type = BigStruct<int>;
    };

    template <>
    struct type_traits<TID::BigStructOfChar>
    {
        static constexpr TID tid = TID::BigStructOfChar;
        static constexpr Kind kind = Kind::BigMutable;
        using type = BigStruct<char>;
    };

    // Property specializations

    template <>
    struct prop_traits<PID::valueInSparse>
    {
        static constexpr TID tid = TID::Integer;
        using declaring_type = SomeClass;
        static constexpr StorageType storage = StorageType::Sparse;
    };

    template <>
    struct prop_traits<PID::valueInField>
    {
        static constexpr TID tid = TID::Integer;
        using declaring_type = SomeClass;
        using accessor_target_t = declaring_type;
        static constexpr StorageType storage = StorageType::Field;
        static constexpr member_getter_t<tid, accessor_target_t> getter = &accessor_target_t::Get_valueInField;
        static constexpr member_setter_t<tid, accessor_target_t> setter = &accessor_target_t::Set_valueInField;
    };

    template <>
    struct prop_traits<PID::valueInGroupSparse>
    {
        static constexpr TID tid = TID::Integer;
        using declaring_type = SomeClass;
        using group_storage_type = GroupStorage;
        using accessor_target_t = group_storage_type;
        static constexpr StorageType storage = StorageType::SparseGroup;
        static constexpr member_getter_t<tid, accessor_target_t> getter = &accessor_target_t::Get_valueInField;
        static constexpr member_setter_t<tid, accessor_target_t> setter = &accessor_target_t::Set_valueInField;
    };

    template <>
    struct prop_traits<PID::smartInSparse>
    {
        static constexpr TID tid = TID::SharedBigStructOfInt;
        using declaring_type = SomeClass;
        static constexpr StorageType storage = StorageType::Sparse;
    };

    template <>
    struct prop_traits<PID::smartInField>
    {
        static constexpr TID tid = TID::SharedBigStructOfInt;
        using declaring_type = SomeClass;
        using accessor_target_t = declaring_type;
        static constexpr StorageType storage = StorageType::Field;
        static constexpr member_getter_t<tid, accessor_target_t> getter = &accessor_target_t::Get_smartInField;
        static constexpr member_setter_t<tid, accessor_target_t> setter = &accessor_target_t::Set_smartInField;
    };

    template <>
    struct prop_traits<PID::smartInGroupSparse>
    {
        static constexpr TID tid = TID::SharedBigStructOfInt;
        using declaring_type = SomeClass;
        using group_storage_type = GroupStorage;
        using accessor_target_t = group_storage_type;
        static constexpr StorageType storage = StorageType::SparseGroup;
        static constexpr member_getter_t<tid, accessor_target_t> getter = &accessor_target_t::Get_smartInField;
        static constexpr member_setter_t<tid, accessor_target_t> setter = &accessor_target_t::Set_smartInField;
    };

    template <>
    struct prop_traits<PID::bigImmutableInSparse>
    {
        static constexpr TID tid = TID::BigStructOfInt;
        using declaring_type = SomeClass;
        static constexpr StorageType storage = StorageType::Sparse;
    };

    template <>
    struct prop_traits<PID::bigImmutableInField>
    {
        static constexpr TID tid = TID::BigStructOfInt;
        using declaring_type = SomeClass;
        using accessor_target_t = declaring_type;
        static constexpr StorageType storage = StorageType::Field;
        static constexpr member_getter_t<tid, accessor_target_t> getter = &accessor_target_t::Get_bigImmutableInField;
        static constexpr member_setter_t<tid, accessor_target_t> setter = &accessor_target_t::Set_bigImmutableInField;
    };

    template <>
    struct prop_traits<PID::bigImmutableInGroupSparse>
    {
        static constexpr TID tid = TID::BigStructOfInt;
        using declaring_type = SomeClass;
        using group_storage_type = GroupStorage;
        using accessor_target_t = group_storage_type;
        static constexpr StorageType storage = StorageType::SparseGroup;
        static constexpr member_getter_t<tid, accessor_target_t> getter = &accessor_target_t::Get_bigImmutableInField;
        static constexpr member_setter_t<tid, accessor_target_t> setter = &accessor_target_t::Set_bigImmutableInField;
    };

    template <>
    struct prop_traits<PID::bigMutableInSparse>
    {
        static constexpr TID tid = TID::BigStructOfChar;
        using declaring_type = SomeClass;
        static constexpr StorageType storage = StorageType::Sparse;
    };

    template <>
    struct prop_traits<PID::bigMutableInField>
    {
        static constexpr TID tid = TID::BigStructOfChar;
        using declaring_type = SomeClass;
        using accessor_target_t = declaring_type;
        static constexpr StorageType storage = StorageType::Field;
        static constexpr member_getter_t<tid, accessor_target_t> getter = &accessor_target_t::Get_bigMutableInField;
        static constexpr member_setter_t<tid, accessor_target_t> setter = &accessor_target_t::Set_bigMutableInField;
    };

    template <>
    struct prop_traits<PID::bigMutableInGroupSparse>
    {
        static constexpr TID tid = TID::BigStructOfChar;
        using declaring_type = SomeClass;
        using group_storage_type = GroupStorage;
        using accessor_target_t = group_storage_type;
        static constexpr StorageType storage = StorageType::SparseGroup;
        static constexpr member_getter_t<tid, accessor_target_t> getter = &accessor_target_t::Get_bigMutableInField;
        static constexpr member_setter_t<tid, accessor_target_t> setter = &accessor_target_t::Set_bigMutableInField;
    };

    // defaults

    struct sparsetables
    {
        sparsetable_from_pid<PID::valueInSparse> m_valueInSparse;
        sparsetable_from_pid<PID::smartInSparse> m_smartInSparse;
        sparsetable_from_pid<PID::bigImmutableInSparse> m_bigImmutableInSparse;
        sparsetable_from_pid<PID::bigMutableInSparse> m_bigMutableInSparse;
        sparsetable<GroupStorage> m_groupStorage;
    };

    struct changehandlerstables
    {
        property_changed_handlers_from_pid<PID::valueInSparse> m_valueInSparse;
        property_changed_handlers_from_pid<PID::valueInField> m_valueInField;
        property_changed_handlers_from_pid<PID::valueInGroupSparse> m_valueInGroupSparse;
        property_changed_handlers_from_pid<PID::smartInSparse> m_smartInSparse;
        property_changed_handlers_from_pid<PID::smartInField> m_smartInField;
        property_changed_handlers_from_pid<PID::smartInGroupSparse> m_smartInGroupSparse;
        property_changed_handlers_from_pid<PID::bigImmutableInSparse> m_bigImmutableInSparse;
        property_changed_handlers_from_pid<PID::bigImmutableInField> m_bigImmutableInField;
        property_changed_handlers_from_pid<PID::bigImmutableInGroupSparse> m_bigImmutableInGroupSparse;
        property_changed_handlers_from_pid<PID::bigMutableInSparse> m_bigMutableInSparse;
        property_changed_handlers_from_pid<PID::bigMutableInField> m_bigMutableInField;
        property_changed_handlers_from_pid<PID::bigMutableInGroupSparse> m_bigMutableInGroupSparse;
    };
}