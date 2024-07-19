// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <optional>
#include <stack_allocator.h>

#define GENERATE_RUNTIMEINFO(pid)                                                               \
{                                                                                               \
    prop_traits<pid>::tid,                                                                      \
    checked_type_erase<prop_traits<pid>::tid>(&Property::id<pid>::Get),                         \
    checked_type_erase<prop_traits<pid>::tid>(&Property::id<pid>::Set),                         \
    checked_type_erase(&details::RegisterHandlerInternal<pid>),                                 \
    checked_type_erase(&details::UnregisterHandlerInternal<pid>),                               \
    &Property::id<pid>::IsSet                                                                   \
}

namespace SimpleProperty
{
    namespace details
    {
        // Number of kinds of handlers which are registered.  Keep this higher than maximum number
        // of handlers to avoid heap allocation when notifying about property change.
        static constexpr std::size_t c_handlerCountHint = 8;

        template <StorageType T> struct tag_storage : std::integral_constant<StorageType, T>::type {};

        // Helpers

        template <typename declaring_type>
        static void MarkValueSetState(
            declaring_type* obj_ptr,
            PID pid,
            bool state)
        {
            (obj_ptr->*sparse_host<declaring_type>::setter)(pid, state);
        }

        // Calls all handlers for changed property
        template <TID tid>
        static void Raise(
            const property_changed_handlers<tid>& handlers,
            objid_t obj,
            typename interface_types_from_tid<tid>::notify_value old_value,
            bool valuesDiffered)
        {
            using PropertyChangedCallback = PropertyChangedCallback<tid>;
            static constexpr std::size_t ArenaSize = c_handlerCountHint * sizeof(PropertyChangedCallback);
            using Alloc = Jupiter::stack_allocator<PropertyChangedCallback, ArenaSize>;

            // Make a copy of handlers in case if it's modified during a call.
            Jupiter::arena<ArenaSize> arena;
            std::vector<PropertyChangedCallback, Alloc> temp(
                handlers.begin(),
                handlers.end(),
                Alloc(arena));

            for (auto& iter : temp)
            {
                if (valuesDiffered || iter.m_alwaysRaiseChangeNotifications)
                {
                    if (iter.m_noValue)
                    {
                        (*iter.m_handlerNoValue)(obj);
                    }
                    else
                    {
                        (*iter.m_handlerWithValue)(obj, old_value);
                    }
                }
            }
        }

        // Helper calling value / const ref value setter (void type::setter(value))
        template <typename T, typename U, typename V>
        static T SetInFieldHelper(
            V* obj,
            void (V::*setter)(U),
            U value,
            T old_value)
        {
            (obj->*setter)(value);
            return old_value;
        }

        // Helper calling big value setter (T type::setter(T&& value))
        // because of last param being const U&, covers both big values (mutable & immutable)

        template <typename T, typename U, typename V>
        static T SetInFieldHelper(
            V* obj,
            T(V::*setter)(T&&),
            T&& value,
            const T&)
        {
            return (obj->*setter)(std::move(value));
        }


        // Getters

        template <PID pid>
        static constexpr typename interface_types_from_pid<pid>::get_return HandleUnset(std::true_type)
        {
            // For values and const references return default.
            return Property::Default<pid>();
        }

        template <PID pid>
        static typename interface_types_from_pid<pid>::get_return HandleUnset(std::false_type)
        {
            // It doesn't make sense to return non-const reference to default, so just bail.
            SP_FAILFAST();
        }

        template <TID tid>
        static typename interface_types_from_tid<tid>::get_return GetImplSparse(
            sparsetable<type_from_tid<tid>>& table,
            const_objid_t obj)
        {
            auto found = table.find(obj);
            ASSERT(found != table.end());
            return found->second;
        }

        template <PID pid>
        static typename interface_types_from_pid<pid>::get_return GetImpl(
            const_objid_t obj,
            tag_storage<StorageType::Sparse>)
        {
            if (Property::id<pid>::IsSet(obj))
            {
                return GetImplSparse<prop_traits<pid>::tid>(
                    GetSparseTable<pid>(obj),
                    obj);
            }
            else
            {
                return HandleUnset<pid>(typename interface_types_from_pid<pid>::has_default());
            }
        }

        template <typename group_storage_type>
        static group_storage_type& GetImplSparseGroup(
            sparsetable<group_storage_type>& table,
            const_objid_t obj)
        {
            auto found = table.find(obj);
            ASSERT(found != table.end());
            return found->second;
        }

        template <PID pid>
        static typename interface_types_from_pid<pid>::get_return GetImpl(
            const_objid_t obj,
            tag_storage<StorageType::SparseGroup>)
        {
            using prop_traits = prop_traits<pid>;

            if (Property::id<pid>::IsSet(obj))
            {
                return (GetImplSparseGroup<typename prop_traits::group_storage_type>(GetSparseTable<pid>(obj), obj).*prop_traits::getter)();
            }
            else
            {
                return HandleUnset<pid>(typename interface_types_from_pid<pid>::has_default());
            }
        }

        template <PID pid>
        static typename interface_types_from_pid<pid>::get_return GetImpl(
            const_objid_t obj,
            tag_storage<StorageType::Field>)
        {
            using prop_traits = prop_traits<pid>;
            return (MapObjIdToInstance<prop_traits::declaring_type>(obj)->*prop_traits::getter)();
        }

        // Setters

        template <typename T>
        using set_result = std::pair<bool, std::optional<T>>;

        template <TID tid>
        static set_result<type_from_tid<tid>> SetImplSparse(
            objid_t obj,
            typename interface_types_from_tid<tid>::set_value value,
            sparsetable<type_from_tid<tid>>& table)
        {
            using optional_t = std::optional<type_from_tid<tid>>;

            auto found = table.lower_bound(obj);

            if (found != table.end() &&
                found->first == obj)
            {
                // Value exists

                if (found->second != value)
                {
                    // ...and is not the same as before, change it and return previous one

                    auto result = std::make_pair(
                        true,
                        std::make_optional<type_from_tid<tid>>(std::move(found->second)));
                    found->second = std::move(value);
                    return result;
                }
            }
            else
            {
                // Does not exist, set it but don't return old one

                table.emplace_hint(found, obj, std::move(value));
                return { true, optional_t() };
            }

            // Value exists, but is the same as current one - don't change it

            return { false, std::make_optional<type_from_tid<tid>>(std::move(value)) };
        }

        template <PID pid>
        static constexpr bool ShouldSet(
            std::false_type,
            const type_from_pid<pid>&,
            const_objid_t)
        {
            // For values without defaults, always set
            return true;
        }

        template <PID pid>
        static constexpr bool ShouldSet(
            std::true_type,
            const type_from_pid<pid>& value,
            const_objid_t obj)
        {
            // For values with default, tests against default and only set if it's different or it's reset to default.
            return (Property::Default<pid>() != value) || Property::id<pid>::IsSet(obj);
        }


        template <PID pid>
        static void SetImpl(
            objid_t obj,
            typename interface_types_from_pid<pid>::set_value value,
            tag_storage<StorageType::Sparse>)
        {
            using prop_traits = prop_traits<pid>;

            if (!ShouldSet<pid>(typename interface_types_from_pid<pid>::has_default(), value, obj))
            {
                return;
            }

            auto result = SetImplSparse<prop_traits::tid>(
                obj,
                std::move(value),
                GetSparseTable<pid>(obj));

            if (result.first)
            {
                // Value has been set.  Mark it and raise notifications.

                MarkValueSetState(
                    MapObjIdToInstance<typename prop_traits::declaring_type>(obj),
                    pid,
                    true);
            }

            Raise<prop_traits::tid>(
                GetHandlersTable<pid>(obj),
                obj,
                result.second ? &result.second.value() : nullptr,
                result.first);
        }

        template <TID tid, typename group_storage_type>
        static set_result<type_from_tid<tid>> SetImplSparseGroup(
            objid_t obj,
            typename interface_types_from_tid<tid>::set_value value,
            sparsetable<group_storage_type>& table,
            member_getter_t<tid, group_storage_type> getter,
            member_setter_t<tid, group_storage_type> setter)
        {
            using optional_t = std::optional<type_from_tid<tid>>;

            auto found = table.lower_bound(obj);

            if (found != table.end() &&
                found->first == obj)
            {
                // Value exists

                // Get a reference to existing value
                auto&& old_value_ref = (found->second.*getter)();

                if (old_value_ref != value)
                {
                    // value is different, change it and return previous one
                    return std::make_pair(
                        true,
                        std::make_optional<type_from_tid<tid>>(
                            SetInFieldHelper<type_from_tid<tid>, decltype(value)>(
                                &found->second,
                                setter,
                                std::move(value),
                                old_value_ref /* this will make a copy in case if this is a reference to field */)));
                }
            }
            else
            {
                // Does not exist, create new storage group, set it but don't return old one

                group_storage_type group;
                (group.*setter)(std::move(value));
                table.emplace_hint(found, obj, std::move(group));
                return { true, optional_t() };
            }

            // Value exists, but is the same as current one - don't change it

            return { false, std::make_optional<type_from_tid<tid>>(std::move(value)) };
        }

        template <PID pid>
        static void SetImpl(
            objid_t obj,
            typename interface_types_from_pid<pid>::set_value value,
            tag_storage<StorageType::SparseGroup>)
        {
            using prop_traits = prop_traits<pid>;

            if (!ShouldSet<pid>(interface_types_from_pid<pid>::has_default(), value, obj))
            {
                return;
            }

            auto result = SetImplSparseGroup<prop_traits::tid, typename prop_traits::group_storage_type>(
                obj,
                std::move(value),
                GetSparseTable<pid>(obj),
                prop_traits::getter,
                prop_traits::setter);

            if (result.first)
            {
                // Value has been set.  Mark it and raise notifications.

                MarkValueSetState(
                    MapObjIdToInstance<prop_traits::declaring_type>(obj),
                    pid,
                    true);
            }

            Raise<prop_traits::tid>(
                GetHandlersTable<pid>(obj),
                obj,
                result.second ? &result.second.value() : nullptr,
                result.first);
        }

        template <PID pid>
        static void SetImpl(
            objid_t obj,
            typename interface_types_from_pid<pid>::set_value value,
            tag_storage<StorageType::Field>)
        {
            using prop_traits = prop_traits<pid>;

            auto&& old_value_ref = GetImpl<pid>(obj, tag_storage<StorageType::Field>());

            bool valuesDiffer = old_value_ref != value;

            // always set the value - we always need a non-const old value
            // to raise with, even if the property value hasn't changed
            auto old_value = SetInFieldHelper<type_from_pid<pid>, decltype(value)>(
                MapObjIdToInstance<prop_traits::declaring_type>(obj),
                prop_traits::setter,
                std::move(value),
                old_value_ref /* this will make a copy in case if this is a reference to field */);

            Raise<prop_traits::tid>(
                GetHandlersTable<pid>(obj),
                obj,
                &old_value,
                valuesDiffer);
        }

        // IsSet

        template <typename declaring_type>
        static bool IsSetImpl(
            const declaring_type* obj_ptr,
            PID pid,
            tag_storage<StorageType::Sparse>)
        {
            return ((obj_ptr->*sparse_host<declaring_type>::query)(pid));
        }

        template <typename declaring_type>
        static bool IsSetImpl(
            const declaring_type* obj_ptr,
            PID pid,
            tag_storage<StorageType::SparseGroup>)
        {
            return ((obj_ptr->*sparse_host<declaring_type>::query)(pid));
        }

        template <typename declaring_type>
        static bool IsSetImpl(
            const declaring_type* obj_ptr,
            PID pid,
            tag_storage<StorageType::Field>)
        {
            return true;
        }

        // DestroyProperty

        template <typename T>
        static void DestroyTable(
            T& table)
        {
            table.clear();
            table.shrink_to_fit();
        }

        template <PID pid>
        static void DestroyPropertyImpl(
            tag_storage<StorageType::Sparse>)
        {
            DestroyTable(GetSparseTable<pid>());
            DestroyTable(GetHandlersTable<pid>());
        }

        template <PID pid>
        static void DestroyPropertyImpl(
            tag_storage<StorageType::SparseGroup>)
        {
            DestroyTable(GetSparseTable<pid>());
            DestroyTable(GetHandlersTable<pid>());
        }

        template <PID pid>
        static void DestroyPropertyImpl(
            tag_storage<StorageType::Field>)
        {
            DestroyTable(GetHandlersTable<pid>());
        }

        // DestroyInstance

        template <PID pid>
        static void DestroyInstanceImpl(
            objid_t obj,
            tag_storage<StorageType::Sparse>)
        {
            using declaring_type = typename prop_traits<pid>::declaring_type;

            if ((MapObjIdToInstance<declaring_type>(obj)->*sparse_host<declaring_type>::query)(pid))
            {
                auto& table = GetSparseTable<pid>(obj);
                table.erase(obj);
            }
        }

        template <PID pid>
        static void DestroyInstanceImpl(
            objid_t obj,
            tag_storage<StorageType::SparseGroup>)
        {
            // all erased on first call to DestroyInstance
            using declaring_type = typename prop_traits<pid>::declaring_type;

            if ((MapObjIdToInstance<declaring_type>(obj)->*sparse_host<declaring_type>::query)(pid))
            {
                auto& table = GetSparseTable<pid>(obj);
                table.erase(obj);
            }
        }

        template <PID pid>
        static void DestroyInstanceImpl(
            objid_t obj,
            tag_storage<StorageType::Field>)
        {}

        // RegisterHandler

        template <TID tid>
        static void RegisterHandlerImpl(
            property_changed_handlers<tid>& handlers,
            typelessfn_t handler,
            bool isNoValue,
            bool alwaysRaiseChangeNotifications)
        {
            auto found = std::find_if(
                handlers.begin(),
                handlers.end(),
                [handler](const auto& entry)
                {
                    return handler == entry.m_handler;
                });

            if (found == handlers.end())
            {
                handlers.emplace_back(handler, isNoValue, alwaysRaiseChangeNotifications);
            }
            else
            {
                ++found->m_count;
            }
        }

        template <PID pid>
        static void RegisterHandlerInternal(
            typelessfn_t handler,
            bool isNoValue,
            bool alwaysRaiseChangeNotifications)
        {
            RegisterHandlerImpl<Property::id<pid>::tid>(
                details::GetHandlersTable<pid>(),
                handler,
                isNoValue,
                alwaysRaiseChangeNotifications);
        }

        // UnregisterHandler

        template <TID tid>
        static void UnregisterHandlerImpl(
            property_changed_handlers<tid>& handlers,
            typelessfn_t handler)
        {
            auto found = std::find_if(
                handlers.begin(),
                handlers.end(),
                [handler](const auto& entry)
                {
                    return handler == entry.m_handler;
                });

            if (found != handlers.end())
            {
                --found->m_count;

                if (found->m_count == 0)
                {
                    handlers.erase(found);
                }
            }
        }

        template <PID pid>
        static void UnregisterHandlerInternal(
            typelessfn_t handler)
        {
            UnregisterHandlerImpl<Property::id<pid>::tid>(
                details::GetHandlersTable<pid>(),
                handler);
        }

#if DBG
        template <PID pid>
        static bool IsCleanedUpImpl(
            const_objid_t obj,
            tag_storage<StorageType::Sparse>)
        {
            auto& table = GetSparseTable<pid>(obj);
            return table.find(obj) == table.end();
        }

        template <PID pid>
        static bool IsCleanedUpImpl(
            const_objid_t obj,
            tag_storage<StorageType::SparseGroup>)
        {
            auto& table = GetSparseTable<pid>(obj);
            return table.find(obj) == table.end();
        }

        template <PID pid>
        static bool IsCleanedUpImpl(
            const_objid_t obj,
            tag_storage<StorageType::Field>)
        {
            return true;
        }
#endif
    }

    // compile-time checked interface
    // Dispatches to specialization based on storage type

    template <PID pid>
    typename interface_types_from_pid<pid>::get_return Property::id<pid>::Get(
        const_objid_t obj)
    {
        return details::GetImpl<pid>(
            obj,
            details::tag_storage<prop_traits::storage>());
    }

    template <PID pid>
    void Property::id<pid>::Set(
        objid_t obj,
        typename interface_types_from_pid<pid>::set_value value)
    {
        details::SetImpl<pid>(
            obj,
            std::move(value),
            details::tag_storage<prop_traits::storage>());
    }

    template <PID pid>
    bool Property::id<pid>::IsSet(
        const_objid_t obj)
    {
        return details::IsSetImpl(
            MapObjIdToInstance<typename prop_traits::declaring_type>(obj),
            pid,
            details::tag_storage<prop_traits::storage>());
    }

    template <PID pid>
    void Property::id<pid>::DestroyProperty()
    {
        details::DestroyPropertyImpl<pid>(
            details::tag_storage<prop_traits::storage>());
    }

    template <PID pid>
    void Property::id<pid>::DestroyInstance(
        objid_t obj)
    {
        details::DestroyInstanceImpl<pid>(
            obj,
            details::tag_storage<prop_traits::storage>());
    }

    template <PID pid>
    void Property::id<pid>::RegisterHandler(
        property_changed_handler<tid> handler,
        bool alwaysRaiseChangeNotifications)
    {
        details::RegisterHandlerInternal<pid>(
            reinterpret_cast<details::typelessfn_t>(handler),
            false,
            alwaysRaiseChangeNotifications);
    }

    template <PID pid>
    void Property::id<pid>::UnregisterHandler(
        property_changed_handler<tid> handler)
    {
        details::UnregisterHandlerInternal<pid>(
            reinterpret_cast<details::typelessfn_t>(handler));
    }

#if DBG
    template <PID pid>
    bool Property::id<pid>::IsCleanedUp(
        const_objid_t obj)
    {
        return details::IsCleanedUpImpl<pid>(
            obj,
            details::tag_storage<prop_traits::storage>());
    }
#endif
}