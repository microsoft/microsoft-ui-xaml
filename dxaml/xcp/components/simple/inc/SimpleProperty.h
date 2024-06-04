// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "vector_map.h"

#ifdef __XAML_UNITTESTS__
    class failfast_exception : public std::exception {};
    #define SP_FAILFAST() throw failfast_exception();
#else
    #define SP_FAILFAST() XAML_FAIL_FAST()
#endif

namespace SimpleProperty
{
    // Key for identifying objects.
    using objid_t       = void*;
    using const_objid_t = const void*;

    // Specifies how the property is stored.
    enum class StorageType
    {
        Field,
        Sparse,
        SparseGroup
    };

    // Specifies how the property appears in the interface (see interface_types).
    // Use Big* to move them instead of copying.  Useful for types which are expensive to construct.
    enum class Kind
    {
        Value,
        ValueByRef,
        BigImmutable,
        BigMutable
    };

    // Underlying type of PID enum.
    using propertyid_numeric_t = std::underlying_type<PID>::type;

    // Property traits.
    // Each property needs to have specialization of this template for its PID value.
    // Expected members / aliases are:
    //   static constexpr TID tid                                           - type index of this property
    //   using declaring_type                                               - type which declares the property
    //   static constexpr StorageType storage                               - storage option for the property
    //   if storage == StorageType::Field, the following need to be declared:
    //     using accessor_target_t = declaring_type                         - field owner is declaring type
    //     static constexpr member_getter_t<tid, accessor_target_t> getter  - name of getter member method (signature depends on type traits' interface types)
    //     static constexpr member_setter_t<tid, accessor_target_t> setter  - name of setter member method (signature depends on type traits' interface types)
    //   if storage == StorageType::Sparse, nothing else needs to be declared.
    //   if storage == StorageType::SparseGroup, the following need to be declared:
    //     using group_storage_type = GroupStorage                          - name of type used for storing the group
    //     using accessor_target_t = group_storage_type                     - field owner is group storage type
    //     static constexpr member_getter_t<tid, accessor_target_t> getter  - address of getter member method (signature depends on type traits' interface types)
    //     static constexpr member_setter_t<tid, accessor_target_t> setter  - address of setter member method (signature depends on type traits' interface types)

    template <PID pid> struct prop_traits {};

    // Type traits.
    // Each type needs to have specialization of this template for its TID value.
    // Expected members / aliases are:
    //   static constexpr TID tid                                           - TID of this type (same as TID being specialized for)
    //   static constexpr Kind kind                                         - Describes how this type will appear in the interface (see interface_types for details)
    //   using type                                                         - Actual C++ type

    template <TID tid> struct type_traits {};

    // Info about a class declaring properties.  Declares methods for accessing sparse property presence flags.  Specialize for each type using sparse storage.
    // Expected members / aliases are:
    // using declaring_type                                                 - same as type being specialized for
    // static constexpr sparse_host_query_t<declaring_type> query           - address of method testing if property value is set
    // static constexpr sparse_host_setter_t<declaring_type> setter         - address of method setting bool presence flag for a proeprty

    template <typename T> struct sparse_host {};

    // Specializations of how properties are used in the interface.
    template <typename T, Kind kind> struct interface_types {};

    template <typename T>
    struct interface_types<T, Kind::Value>
    {
        using get_return = T;
        using set_value = T;
        using set_return = void;
        using notify_value = const T*;
        using has_default = std::true_type;
    };

    template <typename T>
    struct interface_types<T, Kind::ValueByRef>
    {
        using get_return = const T&;
        using set_value = const T&;
        using set_return = void;
        using notify_value = const T*;
        using has_default = std::true_type;
    };

    template <typename T>
    struct interface_types<T, Kind::BigImmutable>
    {
        using get_return = const T&;
        using set_value = T&&;
        using set_return = T;
        using notify_value = const T*;
        using has_default = std::true_type;
    };

    template <typename T>
    struct interface_types<T, Kind::BigMutable>
    {
        using get_return = T&;
        using set_value = T&&;
        using set_return = T;
        using notify_value = T*;
        using has_default = std::false_type;
    };

    // Helpers for getting various info from PID/TID.
    template <TID tid> using type_from_tid = typename type_traits<tid>::type;
    template <PID pid> using type_from_pid = type_from_tid<prop_traits<pid>::tid>;

    template <PID pid> using type_traits_from_pid = type_traits<prop_traits<pid>::tid>;

    template <TID tid> using interface_types_from_tid = interface_types<type_from_tid<tid>, type_traits<tid>::kind>;
    template <PID pid> using interface_types_from_pid = interface_types_from_tid<prop_traits<pid>::tid>;

    // Aliases for field-backed property accessors.
    template <TID tid, typename declaring_type>
    using member_getter_t = typename interface_types_from_tid<tid>::get_return(declaring_type::*)() const;

    template <TID tid, typename declaring_type>
    using member_setter_t = typename interface_types_from_tid<tid>::set_return(declaring_type::*)(typename interface_types_from_tid<tid>::set_value);

    // Aliases for methods used for querying sparse property presence.
    template <typename declaring_type>
    using sparse_host_query_t = bool(declaring_type::*)(PID) const;

    template <typename declaring_type>
    using sparse_host_setter_t = void(declaring_type::*)(PID, bool);

    // Sparse property storage type.
    template <typename T>  using sparsetable = containers::vector_map<objid_t, T>;
    template <PID pid>     using sparsetable_from_pid = sparsetable<type_from_pid<pid>>;

    // forward declare, so hosts can provide their specializations of Get*ForType methods.
    struct sparsetables;
    struct changehandlerstables;

    // GetSparseTablesForType methods should be specialized for each class which declares properties.
    // They should return a structure containing all sparse property storages.
    // Specializations of PickSparseTable will get the appropriate member of this structure based on PID.
    template <typename declaring_type>
    sparsetables& GetSparseTablesForType();

    template <typename declaring_type>
    sparsetables& GetSparseTablesForType(const_objid_t);

    // Property changed notification method and storage of a collection of these.

    using property_changed_no_value_handler             = void(*)(objid_t);
    template <TID tid> using property_changed_handler   = void(*)(objid_t, typename interface_types_from_tid<tid>::notify_value);

    namespace details
    {
        using typelessfn_t = void(*)();

        template <TID tid>
        struct PropertyChangedCallback
        {
            using handler_no_value_t    = property_changed_no_value_handler;
            using handler_t             = property_changed_handler<tid>;

            constexpr PropertyChangedCallback(typelessfn_t handler, bool noValue, bool alwaysRaiseChangeNotifications) noexcept
                : m_handler(handler)
                , m_count(1)
                , m_noValue(noValue)
                , m_alwaysRaiseChangeNotifications(alwaysRaiseChangeNotifications)
            {}

            constexpr PropertyChangedCallback(typelessfn_t handler, bool noValue) noexcept
                : PropertyChangedCallback(handler, true, false)
            {}

            constexpr PropertyChangedCallback(handler_no_value_t handler) noexcept
                : PropertyChangedCallback(handler, true)
            {}

            constexpr PropertyChangedCallback(handler_t handler) noexcept
                : PropertyChangedCallback(handler, false)
            {}

            PropertyChangedCallback(const PropertyChangedCallback& other) = default;

            constexpr PropertyChangedCallback(PropertyChangedCallback&& other) noexcept = default;
            PropertyChangedCallback& operator=(PropertyChangedCallback&& other) noexcept = default;

            union
            {
                handler_no_value_t  m_handlerNoValue;
                handler_t           m_handlerWithValue;
                typelessfn_t        m_handler;
            };

            unsigned m_count    : 30;
            unsigned m_noValue  : 1;
            unsigned m_alwaysRaiseChangeNotifications : 1;
        };
    }

    template <TID tid> using property_changed_handlers = std::vector<details::PropertyChangedCallback<tid>>;
    template <PID pid> using property_changed_handlers_from_pid = property_changed_handlers<prop_traits<pid>::tid>;

    // GetHandlersTablesForType methods should be specialized for each class which declares properties.
    // They should return a structure containing all notification handler collection for all properties.
    // Specializations of PickHandlersTable will get the appropriate member of this structure based on PID.
    template <typename declaring_type>
    changehandlerstables& GetHandlersTablesForType();

    template <typename declaring_type>
    changehandlerstables& GetHandlersTablesForType(const_objid_t);

    // Maps object id to instance of declaring object.  For now, as object ids are just void*, it's a reinterpret_cast.

    template <typename declaring_type>
    declaring_type* MapObjIdToInstance(objid_t);

    template <typename declaring_type>
    const declaring_type* MapObjIdToInstance(const_objid_t);

    // Method translating property id to index for lookup in property table.
    size_t TranslatePIDToIndex(PID pid);

    namespace details
    {
        template <PID pid>
        static auto& PickSparseTable(sparsetables&);

        template <PID pid>
        static auto& GetSparseTable()
        {
            return PickSparseTable<pid>(
                GetSparseTablesForType<typename prop_traits<pid>::declaring_type>());
        }

        template <PID pid>
        static auto& GetSparseTable(const_objid_t obj)
        {
            return PickSparseTable<pid>(
                GetSparseTablesForType<typename prop_traits<pid>::declaring_type>(obj));
        }

        template <PID pid>
        static auto& PickHandlersTable(changehandlerstables&);

        template <PID pid>
        static auto& GetHandlersTable()
        {
            return PickHandlersTable<pid>(
                GetHandlersTablesForType<typename prop_traits<pid>::declaring_type>());
        }

        template <PID pid>
        static auto& GetHandlersTable(const_objid_t obj)
        {
            return PickHandlersTable<pid>(
                GetHandlersTablesForType<typename prop_traits<pid>::declaring_type>(obj));
        }

        // Runtime-checked interface.  Aliases for method signatures stored in run-time table.
        template <TID tid> using getter_t   = typename interface_types_from_tid<tid>::get_return(*)(const_objid_t);
        template <TID tid> using setter_t   = void(*)(objid_t, typename interface_types_from_tid<tid>::set_value);
        using register_t                    = void(*)(typelessfn_t handler, bool, bool);
        using unregister_t                  = void(*)(typelessfn_t handler);

        // Method for sanity-checking that method pointers stored are actually of the correct type.
        template <TID tid>
        static constexpr typelessfn_t checked_type_erase(getter_t<tid> func)
        {
            return reinterpret_cast<typelessfn_t>(func);
        }

        template <TID tid>
        static constexpr typelessfn_t checked_type_erase(setter_t<tid> func)
        {
            return reinterpret_cast<typelessfn_t>(func);
        }

        static inline typelessfn_t checked_type_erase(register_t func)
        {
            return reinterpret_cast<typelessfn_t>(func);
        }

        static inline typelessfn_t checked_type_erase(unregister_t func)
        {
            return reinterpret_cast<typelessfn_t>(func);
        }

        struct RuntimePropertyInfo
        {
            TID typeIndex;
            typelessfn_t getter;
            typelessfn_t setter;
            typelessfn_t registerhandler;
            typelessfn_t unregisterhandler;
            bool(*is_set)(const_objid_t);
        };

        // Table of run-time method pointers indexed by PID.
        extern const RuntimePropertyInfo s_runtimePropertyInfos[];

        template <TID tid>
        static typename interface_types_from_tid<tid>::get_return GetDefaultValue()
        {
            static type_from_tid<tid> s_default = {};
            return s_default;
        }
    }

    class Type
    {
    public:
        // Default values for each type.
        template <TID tid>
        static constexpr typename interface_types_from_tid<tid>::get_return Default()
        {
            return details::GetDefaultValue<tid>();
        }
    };

    class Property
    {
    public:
        // Default values for each property.  By default (!) use value from type.
        template <PID pid>
        static constexpr typename interface_types_from_pid<pid>::get_return Default()
        {
            return Type::Default<prop_traits<pid>::tid>();
        }

        // Compile-time checked interface.  Faster and type-safe.
        template <PID pid>
        class id
        {
            using prop_traits = prop_traits<pid>;
            using interface_types = interface_types_from_pid<pid>;
            friend class Property;

        public:
            using type = type_from_pid<pid>;
            static constexpr TID tid = prop_traits::tid;

            // Returns property value for object id.
            // If value is not explicitly set, the behavior is the following:
            // * if property has storage = Field - value is always set to whatever it was initialized to in host class and will be returned.
            // * if property has storage = Sparse or SparseGroup and kind = Value, ValueByRef or BigImmutable - Property::Default<pid>::value is returned.
            // * if property has storage = Sparse or SparseGroup and kind = BigMutable - exception is thrown (prohibit returning a mutable default object).

            static typename interface_types::get_return Get(const_objid_t obj);

            // Sets value for object id and calls registered change notification handlers.
            // If called with value which is the same as current value, it will not be set and handlers will not be called.
            // If called on a property which has Property::Default<pid> (kind = Value, ValueByRef or BigImmutable) and value is the same as default, it will not be set and handlers will not be called.

            static void Set(objid_t obj, typename interface_types::set_value value);

            // Tests is property value is set for object id.
            // For properties with storage = Field, value is always set.

            static bool IsSet(const_objid_t obj);

            // Registers change notification handler which wants typed old value.
            // For properties with storage = Sparse or SparseGroup, if value is set for the first time the value of previous parameter will be nullptr.
            // Multiple registrations are taken into account, and for each registration there must be a matching unregistration.
            // If alwaysRaiseChangeNotifications is true, the handler will also raise a change notification when a property's new value matches its old value.
            // Multiple registrations must use the same value for alwaysRaiseChangeNotifications.
            static void RegisterHandler(property_changed_handler<tid> handler, bool alwaysRaiseChangeNotifications = false);

            // Unregisters typed change notification handler.
            // Multiple registrations are taken into account, and for each registration there must be a matching unregistration.
            static void UnregisterHandler(property_changed_handler<tid> handler);

#if DBG
            // Sanity check to verify NotifyDestroyed has been called.
            static bool IsCleanedUp(const_objid_t obj);
#endif

        private:
            static void DestroyInstance(objid_t obj);
            static void DestroyProperty();
        };

        // Run-time type checked interface.  If possible, use compile-time versions for better performance and type-safety.
        template <TID tid>
        class as
        {
            using interface_types = interface_types_from_tid<tid>;

            static void validate_type(const details::RuntimePropertyInfo& info)
            {
                if (info.typeIndex != tid)
                {
                    SP_FAILFAST();
                }
            }

        public:
            static typename interface_types::get_return Get(const_objid_t obj, PID pid)
            {
                const details::RuntimePropertyInfo& info = details::s_runtimePropertyInfos[TranslatePIDToIndex(pid)];
                validate_type(info);
                return reinterpret_cast<details::getter_t<tid>>(info.getter)(obj);
            }

            static void Set(objid_t obj, PID pid, typename interface_types::set_value value)
            {
                const details::RuntimePropertyInfo& info = details::s_runtimePropertyInfos[TranslatePIDToIndex(pid)];
                validate_type(info);
                reinterpret_cast<details::setter_t<tid>>(info.setter)(obj, std::move(value));
            }

            static void RegisterHandler(PID pid, property_changed_handler<tid> handler)
            {
                const details::RuntimePropertyInfo& info = details::s_runtimePropertyInfos[TranslatePIDToIndex(pid)];
                validate_type(info);
                return reinterpret_cast<details::register_t>(info.registerhandler)(reinterpret_cast<details::typelessfn_t>(handler), false, false);
            }

            static void UnregisterHandler(PID pid, property_changed_handler<tid> handler)
            {
                const details::RuntimePropertyInfo& info = details::s_runtimePropertyInfos[TranslatePIDToIndex(pid)];
                validate_type(info);
                return reinterpret_cast<details::unregister_t>(info.unregisterhandler)(reinterpret_cast<details::typelessfn_t>(handler));
            }
        };

        // Run-time test if property value is set for object id.
        // If possible, use Property::id<pid>::IsSet() instead.

        static bool IsSet(const_objid_t obj, PID pid)
        {
            const details::RuntimePropertyInfo& info = details::s_runtimePropertyInfos[TranslatePIDToIndex(pid)];
            return info.is_set(obj);
        }

        // Registers change notification handler which does not want old value or change notifications when the old and new values are the same.
        // Multiple registrations are taken into account, and for each registration there must be a matching unregistration.
        static void RegisterHandler(PID pid, property_changed_no_value_handler handler)
        {
            const details::RuntimePropertyInfo& info = details::s_runtimePropertyInfos[TranslatePIDToIndex(pid)];
            return reinterpret_cast<details::register_t>(info.registerhandler)(reinterpret_cast<details::typelessfn_t>(handler), true, false);
        }

        // Unregisters change notification handler which doesn't want old value.
        // Multiple registrations are taken into account, and for each registration there must be a matching unregistration.
        static void UnregisterHandler(PID pid, property_changed_no_value_handler handler)
        {
            const details::RuntimePropertyInfo& info = details::s_runtimePropertyInfos[TranslatePIDToIndex(pid)];
            return reinterpret_cast<details::unregister_t>(info.unregisterhandler)(reinterpret_cast<details::typelessfn_t>(handler));
        }

        // Method called by host class when it no longer needs its properties being stored (usually called from dtor).
        // Template parameter T is the specific class declaring properties to be cleaned up.

        template <typename T>
        static void NotifyDestroyed(objid_t obj);

        // Cleanup method for removing all properties and handlers and releasing table resources.

        static void DestroyAllProperties();

        // Sanity check to verify NotifyDestroyed has been called.

        static bool AreAllCleanedUp(const_objid_t obj);
    };
}