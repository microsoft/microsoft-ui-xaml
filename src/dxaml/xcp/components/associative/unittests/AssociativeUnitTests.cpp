// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AssociativeUnitTests.h"
#include "AssociativeStorage.h"
#include <XamlLogging.h>
#include <cstdlib>
#include <memory>

using namespace WEX::Common;
using namespace AssociativeStorage;

static constexpr size_t c_stressIterationCount = 500;

struct CustomType
{
    struct CtorThrowGuard
    {
        CtorThrowGuard()
        {
            CustomType::s_throwInCtor = true;
        }

        ~CtorThrowGuard()
        {
            CustomType::s_throwInCtor = false;
        }
    };

    struct MoveCtorThrowGuard
    {
        MoveCtorThrowGuard()
        {
            CustomType::s_throwInMoveCtor = true;
        }

        ~MoveCtorThrowGuard()
        {
            CustomType::s_throwInMoveCtor = false;
        }
    };

    CustomType()
    {
        ++s_ctorCalls;

        if (s_throwInCtor)
        {
            throw std::exception();
        }
    }

#pragma warning(suppress: 26439) // This kind of function should not throw. Declare it 'noexcept'
    CustomType(CustomType&& other) // Intentionally not noexcept, because it throws.
    {
        ++s_moveCtorCalls;

        if (s_throwInMoveCtor)
        {
            throw std::exception();
        }

        value = other.value;
    }

    ~CustomType()
    {
        ++s_dtorCalls;
    }

    static void ResetCounters()
    {
        s_ctorCalls = 0;
        s_moveCtorCalls = 0;
        s_dtorCalls = 0;
    }

    int value = 0;

    static bool s_throwInCtor;
    static bool s_throwInMoveCtor;
    static int s_ctorCalls;
    static int s_moveCtorCalls;
    static int s_dtorCalls;
};

bool CustomType::s_throwInCtor = false;
bool CustomType::s_throwInMoveCtor = false;
int CustomType::s_ctorCalls = 0;
int CustomType::s_moveCtorCalls = 0;
int CustomType::s_dtorCalls = 0;

namespace AssociativeStorage
{
    enum class FieldEnum0 : uint8_t
    {
        field0,
        field1,
        field2,
        field3,
        field4,
        field5,
        field6,
        field7,
        Sentinel
    };

    template <>
    struct FieldInfo<FieldEnum0, FieldEnum0::field0>
    {
        using StorageType = double;
    };

    template <>
    struct FieldInfo<FieldEnum0, FieldEnum0::field1>
    {
        using StorageType = double;
    };

    template <>
    struct FieldInfo<FieldEnum0, FieldEnum0::field2>
    {
        using StorageType = std::unique_ptr<CustomType>;
    };

    template <>
    struct FieldInfo<FieldEnum0, FieldEnum0::field3>
    {
        using StorageType = std::unique_ptr<CustomType>;
    };

    template <>
    struct FieldInfo<FieldEnum0, FieldEnum0::field4>
    {
        using StorageType = float;
    };

    template <>
    struct FieldInfo<FieldEnum0, FieldEnum0::field5>
    {
        using StorageType = float;
    };

    template <>
    struct FieldInfo<FieldEnum0, FieldEnum0::field6>
    {
        using StorageType = CustomType;
    };

    template <>
    struct FieldInfo<FieldEnum0, FieldEnum0::field7>
    {
        using StorageType = uint8_t;
    };

    template <>
    struct FieldRuntimeInfoArray<FieldEnum0>
    {
        static constexpr FieldRuntimeInfoArrayType<FieldEnum0> fields =
        {
            GenerateFieldRuntimeInfo<FieldEnum0, FieldEnum0::field0>(),
            GenerateFieldRuntimeInfo<FieldEnum0, FieldEnum0::field1>(),
            GenerateFieldRuntimeInfo<FieldEnum0, FieldEnum0::field2>(),
            GenerateFieldRuntimeInfo<FieldEnum0, FieldEnum0::field3>(),
            GenerateFieldRuntimeInfo<FieldEnum0, FieldEnum0::field4>(),
            GenerateFieldRuntimeInfo<FieldEnum0, FieldEnum0::field5>(),
            GenerateFieldRuntimeInfo<FieldEnum0, FieldEnum0::field6>(),
            GenerateFieldRuntimeInfo<FieldEnum0, FieldEnum0::field7>(),
        };
    };

    template <> Detail::VariableStorageBlock<Detail::Bitfield<FieldEnum0>>::OffsetLookupTable Detail::VariableStorageBlock<Detail::Bitfield<FieldEnum0>>::s_lookupTable;

    enum class FieldEnum1 : uint16_t
    {
        field0,
        field1,
        field2,
        field3,
        field4,
        field5,
        field6,
        field7,
        field8,
        field9,
        field10,
        field11,
        field12,
        field13,
        field14,
        field15,
        Sentinel
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field0>
    {
        using StorageType = double;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field1>
    {
        using StorageType = double;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field2>
    {
        using StorageType = double;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field3>
    {
        using StorageType = double;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field4>
    {
        using StorageType = std::unique_ptr<CustomType>;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field5>
    {
        using StorageType = std::unique_ptr<CustomType>;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field6>
    {
        using StorageType = std::unique_ptr<CustomType>;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field7>
    {
        using StorageType = std::unique_ptr<CustomType>;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field8>
    {
        using StorageType = float;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field9>
    {
        using StorageType = float;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field10>
    {
        using StorageType = float;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field11>
    {
        using StorageType = float;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field12>
    {
        using StorageType = CustomType;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field13>
    {
        using StorageType = CustomType;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field14>
    {
        using StorageType = uint8_t;
    };

    template <>
    struct FieldInfo<FieldEnum1, FieldEnum1::field15>
    {
        using StorageType = uint8_t;
    };

    template <>
    struct FieldRuntimeInfoArray<FieldEnum1>
    {
        static constexpr FieldRuntimeInfoArrayType<FieldEnum1> fields =
        {
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field0>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field1>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field2>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field3>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field4>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field5>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field6>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field7>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field8>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field9>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field10>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field11>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field12>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field13>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field14>(),
            GenerateFieldRuntimeInfo<FieldEnum1, FieldEnum1::field15>(),
        };
    };

    template <> Detail::VariableStorageBlock<Detail::Bitfield<FieldEnum1>>::OffsetLookupTable Detail::VariableStorageBlock<Detail::Bitfield<FieldEnum1>>::s_lookupTable;
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Associative {

    using namespace AssociativeStorage;

    bool AssociativeUnitTests::ClassSetup()
    {
        return true;
    }

    void AssociativeUnitTests::LocalStorage_Allocations()
    {
        {
            // None allocated, none exist

            LocalStorage<FieldEnum0> storage;

            auto f0 = storage.Get<FieldEnum0::field0>();
            VERIFY_IS_NULL(f0);

            auto f1 = storage.Get<FieldEnum0::field1>();
            VERIFY_IS_NULL(f1);

            auto f2 = storage.Get<FieldEnum0::field2>();
            VERIFY_IS_NULL(f2);

            auto f3 = storage.Get<FieldEnum0::field3>();
            VERIFY_IS_NULL(f3);

            auto f4 = storage.Get<FieldEnum0::field4>();
            VERIFY_IS_NULL(f4);

            auto f5 = storage.Get<FieldEnum0::field5>();
            VERIFY_IS_NULL(f5);

            auto f6 = storage.Get<FieldEnum0::field6>();
            VERIFY_IS_NULL(f6);

            auto f7 = storage.Get<FieldEnum0::field7>();
            VERIFY_IS_NULL(f7);
        }

        {
            // Allocate one, others don't not exist.

            LocalStorage<FieldEnum0> storage;

            auto& f4r = storage.Ensure<FieldEnum0::field4>();
            f4r = 1234.5f;

            auto f0 = storage.Get<FieldEnum0::field0>();
            VERIFY_IS_NULL(f0);

            auto f1 = storage.Get<FieldEnum0::field1>();
            VERIFY_IS_NULL(f1);

            auto f2 = storage.Get<FieldEnum0::field2>();
            VERIFY_IS_NULL(f2);

            auto f3 = storage.Get<FieldEnum0::field3>();
            VERIFY_IS_NULL(f3);

            auto f4 = storage.Get<FieldEnum0::field4>();
            VERIFY_IS_NOT_NULL(f4);
            VERIFY_ARE_EQUAL(1234.5f, *f4);

            auto f5 = storage.Get<FieldEnum0::field5>();
            VERIFY_IS_NULL(f5);

            auto f6 = storage.Get<FieldEnum0::field6>();
            VERIFY_IS_NULL(f6);

            auto f7 = storage.Get<FieldEnum0::field7>();
            VERIFY_IS_NULL(f7);
        }

        {
            // Previously allocated value should be the same even though it moved

            LocalStorage<FieldEnum0> storage;

            auto& f0r = storage.Ensure<FieldEnum0::field0>();
            f0r = 3.1415;

            auto f0 = storage.Get<FieldEnum0::field0>();
            VERIFY_ARE_EQUAL(3.1415, *f0);

            std::unique_ptr<CustomType> obj(new CustomType());
            CustomType* ptr = obj.get();
            obj->value = 99;

            // Add value to cause realloc
            auto& f3 = storage.Ensure<FieldEnum0::field3>();
            f3 = std::move(obj);

            auto f0_1 = storage.Get<FieldEnum0::field0>();
            VERIFY_ARE_EQUAL(3.1415, *f0_1);
            VERIFY_ARE_NOT_EQUAL(f0, f0_1);

            auto& f4r = storage.Ensure<FieldEnum0::field4>();
            f4r = 1921.0f;

            auto f0_2 = storage.Get<FieldEnum0::field0>();
            VERIFY_ARE_EQUAL(3.1415, *f0_2);
            VERIFY_ARE_NOT_EQUAL(f0_2, f0_1);

            auto f3_0 = storage.Get<FieldEnum0::field3>();
            VERIFY_ARE_EQUAL(ptr, f3_0->get());
        }
    }

    void AssociativeUnitTests::LocalStorage_CtorsAndDtors()
    {
        {
            CustomType::ResetCounters();

            LocalStorage<FieldEnum0> storage;

            // Ensure calls ctor

            &storage.Ensure<FieldEnum0::field6>();
            VERIFY_ARE_EQUAL(1, CustomType::s_ctorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_moveCtorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_dtorCalls);
            CustomType::ResetCounters();

            // Remove calls dtor

            storage.Remove<FieldEnum0::field6>();
            VERIFY_ARE_EQUAL(0, CustomType::s_ctorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_moveCtorCalls);
            VERIFY_ARE_EQUAL(1, CustomType::s_dtorCalls);
            CustomType::ResetCounters();

            // Ensure calls ctor again, instances are different

            &storage.Ensure<FieldEnum0::field6>();
            VERIFY_ARE_EQUAL(1, CustomType::s_ctorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_moveCtorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_dtorCalls);
            CustomType::ResetCounters();
        }

        {
            CustomType::ResetCounters();

            LocalStorage<FieldEnum0> storage;

            // Ensure calls ctor

            storage.Ensure<FieldEnum0::field6>();
            VERIFY_ARE_EQUAL(1, CustomType::s_ctorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_moveCtorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_dtorCalls);
            CustomType::ResetCounters();

            // Reallocation causes move ctor and dtor calls

            &storage.Ensure<FieldEnum0::field0>();
            VERIFY_ARE_EQUAL(0, CustomType::s_ctorCalls);
            VERIFY_ARE_EQUAL(1, CustomType::s_moveCtorCalls);
            VERIFY_ARE_EQUAL(1, CustomType::s_dtorCalls);
            CustomType::ResetCounters();
        }
    }

    void AssociativeUnitTests::LocalStorage_Preallocation()
    {
        {
            // Preallocate works before any allocations

            using StorageType = LocalStorage<FieldEnum0>;

            StorageType storage;

            StorageType::Bitfield bitfield;
            bitfield.Add(FieldEnum0::field1);
            bitfield.Add(FieldEnum0::field2);
            storage.PreAllocate(bitfield);

            auto f0 = storage.Get<FieldEnum0::field0>();
            VERIFY_IS_NULL(f0);

            auto f1 = storage.Get<FieldEnum0::field1>();
            VERIFY_IS_NOT_NULL(f1);

            auto f2 = storage.Get<FieldEnum0::field2>();
            VERIFY_IS_NOT_NULL(f2);

            auto f3 = storage.Get<FieldEnum0::field3>();
            VERIFY_IS_NULL(f3);

            auto f4 = storage.Get<FieldEnum0::field4>();
            VERIFY_IS_NULL(f4);

            auto f5 = storage.Get<FieldEnum0::field5>();
            VERIFY_IS_NULL(f5);

            auto f6 = storage.Get<FieldEnum0::field6>();
            VERIFY_IS_NULL(f6);

            auto f7 = storage.Get<FieldEnum0::field7>();
            VERIFY_IS_NULL(f7);
        }

        {
            // Preallocate works after allocation

            using StorageType = LocalStorage<FieldEnum0>;

            StorageType storage;

            auto& f0r = storage.Ensure<FieldEnum0::field0>();
            f0r = 3.1415;

            StorageType::Bitfield bitfield;
            bitfield.Add(FieldEnum0::field1);
            bitfield.Add(FieldEnum0::field2);
            storage.PreAllocate(bitfield);

            auto f0 = storage.Get<FieldEnum0::field0>();
            VERIFY_IS_NULL(f0);

            auto f1 = storage.Get<FieldEnum0::field1>();
            VERIFY_IS_NOT_NULL(f1);

            auto f2 = storage.Get<FieldEnum0::field2>();
            VERIFY_IS_NOT_NULL(f2);

            auto f3 = storage.Get<FieldEnum0::field3>();
            VERIFY_IS_NULL(f3);

            auto f4 = storage.Get<FieldEnum0::field4>();
            VERIFY_IS_NULL(f4);

            auto f5 = storage.Get<FieldEnum0::field5>();
            VERIFY_IS_NULL(f5);

            auto f6 = storage.Get<FieldEnum0::field6>();
            VERIFY_IS_NULL(f6);

            auto f7 = storage.Get<FieldEnum0::field7>();
            VERIFY_IS_NULL(f7);
        }
    }

    void AssociativeUnitTests::LocalStorage_Exceptions()
    {
        {
            LocalStorage<FieldEnum1> storage;

            // Create CustomType by value
            auto f12_r = &storage.Ensure<FieldEnum1::field12>();
            f12_r->value = 13;

            CustomType::ResetCounters();
            CustomType::CtorThrowGuard guard;

            VERIFY_THROWS(storage.Ensure<FieldEnum1::field13>(), std::exception);

            // Only throwing ctor was called, so nothing needs to be cleaned up.
            VERIFY_ARE_EQUAL(1, CustomType::s_ctorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_moveCtorCalls);
            VERIFY_ARE_EQUAL(0, CustomType::s_dtorCalls);

            auto f12 = storage.Get<FieldEnum1::field12>();
            VERIFY_IS_NOT_NULL(f12);
            VERIFY_ARE_EQUAL(f12, f12_r);
            VERIFY_ARE_EQUAL(f12->value, f12_r->value);
        }
        {
            // This scenario is not supported very well.
            // When move ctor throws, at the very least verify newly constructed object was deleted...

            LocalStorage<FieldEnum1> storage;

            // Create CustomType by value
            auto f12_r = &storage.Ensure<FieldEnum1::field12>();
            f12_r->value = 13;

            CustomType::ResetCounters();

            // move ctor of field12 will throw
            CustomType::MoveCtorThrowGuard guard;

            // field13 does not exist, but its dtor was called.
            VERIFY_THROWS(storage.Ensure<FieldEnum1::field13>(), std::exception);

            VERIFY_ARE_EQUAL(1, CustomType::s_ctorCalls);
            VERIFY_ARE_EQUAL(1, CustomType::s_moveCtorCalls);
            VERIFY_ARE_EQUAL(1, CustomType::s_dtorCalls);

            auto f12 = storage.Get<FieldEnum1::field12>();
            VERIFY_IS_NOT_NULL(f12);
            VERIFY_ARE_EQUAL(f12, f12_r);
            VERIFY_ARE_EQUAL(f12->value, f12_r->value);
        }
    }

    template <typename T>
    struct FieldState
    {
        void set(const T& value)
        {
            m_value = value;
            m_set = true;
        }

        void clear()
        {
            m_set = false;
        }

        T m_value = {};
        bool m_set = false;
    };

    struct ReferenceState8
    {
        FieldState<double> f0;
        FieldState<double> f1;
        FieldState<CustomType*> f2;
        FieldState<CustomType*> f3;
        FieldState<float> f4;
        FieldState<float> f5;
        FieldState<int> f6;
        FieldState<uint8_t> f7;
    };

    struct ReferenceState16
    {
        FieldState<double> f0;
        FieldState<double> f1;
        FieldState<double> f2;
        FieldState<double> f3;
        FieldState<CustomType*> f4;
        FieldState<CustomType*> f5;
        FieldState<CustomType*> f6;
        FieldState<CustomType*> f7;
        FieldState<float> f8;
        FieldState<float> f9;
        FieldState<float> f10;
        FieldState<float> f11;
        FieldState<int> f12;
        FieldState<int> f13;
        FieldState<uint8_t> f14;
        FieldState<uint8_t> f15;
    };

    enum class Action
    {
        Ensure,
        Remove
    };

    Action GetRandomAction()
    {
        return (Action)(rand() % 2);
    }

    template <typename TEnumType>
    TEnumType GetRandomField()
    {
        return (TEnumType)(rand() % static_cast<size_t>(TEnumType::Sentinel));
    }

    template <typename TFieldEnum, TFieldEnum field>
    void EnsureValueField(LocalStorage<TFieldEnum>& storage, FieldState<typename FieldInfo<TFieldEnum, field>::StorageType>& reference)
    {
        auto& valueRef = storage.Ensure<field>();
        valueRef = (FieldInfo<TFieldEnum, field>::StorageType)rand();
        reference.set(valueRef);
    }

    template <typename TFieldEnum, TFieldEnum field>
    void EnsureSmartPtrField(LocalStorage<TFieldEnum>& storage, FieldState<CustomType*>& reference)
    {
        auto& valueRef = storage.Ensure<field>();
        std::unique_ptr<CustomType> value(new CustomType);
        value->value = rand();
        reference.set(value.get());
        valueRef = std::move(value);
    }

    template <typename TFieldEnum, TFieldEnum field>
    void EnsureCustomField(LocalStorage<TFieldEnum>& storage, FieldState<int>& reference)
    {
        auto& valueRef = storage.Ensure<field>();
        valueRef.value = rand();
        reference.set(valueRef.value);
    }

    template <typename TFieldEnum, TFieldEnum field>
    void RemoveValueField(LocalStorage<TFieldEnum>& storage, FieldState<typename FieldInfo<TFieldEnum, field>::StorageType>& reference)
    {
        storage.Remove<field>();
        reference.clear();
    }

    template <typename TFieldEnum, TFieldEnum field>
    void RemoveSmartPtrField(LocalStorage<TFieldEnum>& storage, FieldState<CustomType*>& reference)
    {
        storage.Remove<field>();
        reference.clear();
    }

    template <typename TFieldEnum, TFieldEnum field>
    void RemoveCustomField(LocalStorage<TFieldEnum>& storage, FieldState<int>& reference)
    {
        storage.Remove<field>();
        reference.clear();
    }

    template <typename TFieldEnum, TFieldEnum field>
    void ValidateValueField(LocalStorage<TFieldEnum>& storage, FieldState<typename FieldInfo<TFieldEnum, field>::StorageType>& reference)
    {
        auto valuePtr = storage.Get<field>();

        if (reference.m_set)
        {
            VERIFY_IS_NOT_NULL(valuePtr);
            VERIFY_ARE_EQUAL(*valuePtr, reference.m_value);
        }
        else
        {
            VERIFY_IS_NULL(valuePtr);
        }
    }

    template <typename TFieldEnum, TFieldEnum field>
    void ValidateSmartPtrField(LocalStorage<TFieldEnum>& storage, FieldState<CustomType*>& reference)
    {
        auto valuePtr = storage.Get<field>();

        if (reference.m_set)
        {
            VERIFY_IS_NOT_NULL(valuePtr);
            VERIFY_ARE_EQUAL(valuePtr->get(), reference.m_value);
        }
        else
        {
            VERIFY_IS_NULL(valuePtr);
        }
    }

    template <typename TFieldEnum, TFieldEnum field>
    void ValidateCustomField(LocalStorage<TFieldEnum>& storage, FieldState<int>& reference)
    {
        auto valuePtr = storage.Get<field>();

        if (reference.m_set)
        {
            VERIFY_IS_NOT_NULL(valuePtr);
            VERIFY_ARE_EQUAL(valuePtr->value, reference.m_value);
        }
        else
        {
            VERIFY_IS_NULL(valuePtr);
        }
    }

    void EnsureAction(LocalStorage<FieldEnum0>& storage, FieldEnum0 field, ReferenceState8& reference)
    {
        switch (field)
        {
        case FieldEnum0::field0:
            EnsureValueField<FieldEnum0, FieldEnum0::field0>(storage, reference.f0);
            break;

        case FieldEnum0::field1:
            EnsureValueField<FieldEnum0, FieldEnum0::field1>(storage, reference.f1);
            break;

        case FieldEnum0::field2:
            EnsureSmartPtrField<FieldEnum0, FieldEnum0::field2>(storage, reference.f2);
            break;

        case FieldEnum0::field3:
            EnsureSmartPtrField<FieldEnum0, FieldEnum0::field3>(storage, reference.f3);
            break;

        case FieldEnum0::field4:
            EnsureValueField<FieldEnum0, FieldEnum0::field4>(storage, reference.f4);
            break;

        case FieldEnum0::field5:
            EnsureValueField<FieldEnum0, FieldEnum0::field5>(storage, reference.f5);
            break;

        case FieldEnum0::field6:
            EnsureCustomField<FieldEnum0, FieldEnum0::field6>(storage, reference.f6);
            break;

        case FieldEnum0::field7:
            EnsureValueField<FieldEnum0, FieldEnum0::field7>(storage, reference.f7);
            break;
        }
    }

    void RemoveAction(LocalStorage<FieldEnum0>& storage, FieldEnum0 field, ReferenceState8& reference)
    {
        switch (field)
        {
        case FieldEnum0::field0:
            RemoveValueField<FieldEnum0, FieldEnum0::field0>(storage, reference.f0);
            break;

        case FieldEnum0::field1:
            RemoveValueField<FieldEnum0, FieldEnum0::field1>(storage, reference.f1);
            break;

        case FieldEnum0::field2:
            RemoveSmartPtrField<FieldEnum0, FieldEnum0::field2>(storage, reference.f2);
            break;

        case FieldEnum0::field3:
            RemoveSmartPtrField<FieldEnum0, FieldEnum0::field3>(storage, reference.f3);
            break;

        case FieldEnum0::field4:
            RemoveValueField<FieldEnum0, FieldEnum0::field4>(storage, reference.f4);
            break;

        case FieldEnum0::field5:
            RemoveValueField<FieldEnum0, FieldEnum0::field5>(storage, reference.f5);
            break;

        case FieldEnum0::field6:
            RemoveCustomField<FieldEnum0, FieldEnum0::field6>(storage, reference.f6);
            break;

        case FieldEnum0::field7:
            RemoveValueField<FieldEnum0, FieldEnum0::field7>(storage, reference.f7);
            break;
        }
    }

    void EnsureAction(LocalStorage<FieldEnum1>& storage, FieldEnum1 field, ReferenceState16& reference)
    {
        switch (field)
        {
        case FieldEnum1::field0:
            EnsureValueField<FieldEnum1, FieldEnum1::field0>(storage, reference.f0);
            break;

        case FieldEnum1::field1:
            EnsureValueField<FieldEnum1, FieldEnum1::field1>(storage, reference.f1);
            break;

        case FieldEnum1::field2:
            EnsureValueField<FieldEnum1, FieldEnum1::field2>(storage, reference.f2);
            break;

        case FieldEnum1::field3:
            EnsureValueField<FieldEnum1, FieldEnum1::field3>(storage, reference.f3);
            break;

        case FieldEnum1::field4:
            EnsureSmartPtrField<FieldEnum1, FieldEnum1::field4>(storage, reference.f4);
            break;

        case FieldEnum1::field5:
            EnsureSmartPtrField<FieldEnum1, FieldEnum1::field5>(storage, reference.f5);
            break;

        case FieldEnum1::field6:
            EnsureSmartPtrField<FieldEnum1, FieldEnum1::field6>(storage, reference.f6);
            break;

        case FieldEnum1::field7:
            EnsureSmartPtrField<FieldEnum1, FieldEnum1::field7>(storage, reference.f7);
            break;

        case FieldEnum1::field8:
            EnsureValueField<FieldEnum1, FieldEnum1::field8>(storage, reference.f8);
            break;

        case FieldEnum1::field9:
            EnsureValueField<FieldEnum1, FieldEnum1::field9>(storage, reference.f9);
            break;

        case FieldEnum1::field10:
            EnsureValueField<FieldEnum1, FieldEnum1::field10>(storage, reference.f10);
            break;

        case FieldEnum1::field11:
            EnsureValueField<FieldEnum1, FieldEnum1::field11>(storage, reference.f11);
            break;

        case FieldEnum1::field12:
            EnsureCustomField<FieldEnum1, FieldEnum1::field12>(storage, reference.f12);
            break;

        case FieldEnum1::field13:
            EnsureCustomField<FieldEnum1, FieldEnum1::field13>(storage, reference.f13);
            break;

        case FieldEnum1::field14:
            EnsureValueField<FieldEnum1, FieldEnum1::field14>(storage, reference.f14);
            break;

        case FieldEnum1::field15:
            EnsureValueField<FieldEnum1, FieldEnum1::field15>(storage, reference.f15);
            break;
        }
    }

    void RemoveAction(LocalStorage<FieldEnum1>& storage, FieldEnum1 field, ReferenceState16& reference)
    {
        switch (field)
        {
        case FieldEnum1::field0:
            RemoveValueField<FieldEnum1, FieldEnum1::field0>(storage, reference.f0);
            break;

        case FieldEnum1::field1:
            RemoveValueField<FieldEnum1, FieldEnum1::field1>(storage, reference.f1);
            break;

        case FieldEnum1::field2:
            RemoveValueField<FieldEnum1, FieldEnum1::field2>(storage, reference.f2);
            break;

        case FieldEnum1::field3:
            RemoveValueField<FieldEnum1, FieldEnum1::field3>(storage, reference.f3);
            break;

        case FieldEnum1::field4:
            RemoveSmartPtrField<FieldEnum1, FieldEnum1::field4>(storage, reference.f4);
            break;

        case FieldEnum1::field5:
            RemoveSmartPtrField<FieldEnum1, FieldEnum1::field5>(storage, reference.f5);
            break;

        case FieldEnum1::field6:
            RemoveSmartPtrField<FieldEnum1, FieldEnum1::field6>(storage, reference.f6);
            break;

        case FieldEnum1::field7:
            RemoveSmartPtrField<FieldEnum1, FieldEnum1::field7>(storage, reference.f7);
            break;

        case FieldEnum1::field8:
            RemoveValueField<FieldEnum1, FieldEnum1::field8>(storage, reference.f8);
            break;

        case FieldEnum1::field9:
            RemoveValueField<FieldEnum1, FieldEnum1::field9>(storage, reference.f9);
            break;

        case FieldEnum1::field10:
            RemoveValueField<FieldEnum1, FieldEnum1::field10>(storage, reference.f10);
            break;

        case FieldEnum1::field11:
            RemoveValueField<FieldEnum1, FieldEnum1::field11>(storage, reference.f11);
            break;

        case FieldEnum1::field12:
            RemoveCustomField<FieldEnum1, FieldEnum1::field12>(storage, reference.f12);
            break;

        case FieldEnum1::field13:
            RemoveCustomField<FieldEnum1, FieldEnum1::field13>(storage, reference.f13);
            break;

        case FieldEnum1::field14:
            RemoveValueField<FieldEnum1, FieldEnum1::field14>(storage, reference.f14);
            break;

        case FieldEnum1::field15:
            RemoveValueField<FieldEnum1, FieldEnum1::field15>(storage, reference.f15);
            break;
        }
    }

    void AssociativeUnitTests::LocalStorageStressTests_8()
    {
        using StorageType = LocalStorage<FieldEnum0>;

        StorageType storage;
        ReferenceState8 reference;

        srand(31415);

        for (size_t i = 0; i < c_stressIterationCount; ++i)
        {
            #pragma warning(suppress: 6328 6340) // size mismatch, sign mismatch
            LOG_OUTPUT(L"Iteration %d", i);

            auto targetField = GetRandomField<FieldEnum0>();

            switch (GetRandomAction())
            {
                case Action::Ensure:
                    EnsureAction(storage, targetField, reference);
                    break;

                case Action::Remove:
                    RemoveAction(storage, targetField, reference);
                    break;
            }

            ValidateValueField<FieldEnum0, FieldEnum0::field0>(storage, reference.f0);
            ValidateValueField<FieldEnum0, FieldEnum0::field1>(storage, reference.f1);
            ValidateSmartPtrField<FieldEnum0, FieldEnum0::field2>(storage, reference.f2);
            ValidateSmartPtrField<FieldEnum0, FieldEnum0::field3>(storage, reference.f3);
            ValidateValueField<FieldEnum0, FieldEnum0::field4>(storage, reference.f4);
            ValidateValueField<FieldEnum0, FieldEnum0::field5>(storage, reference.f5);
            ValidateCustomField<FieldEnum0, FieldEnum0::field6>(storage, reference.f6);
            ValidateValueField<FieldEnum0, FieldEnum0::field7>(storage, reference.f7);
        }
    }

    void AssociativeUnitTests::LocalStorageStressTests_16()
    {
        using StorageType = LocalStorage<FieldEnum1>;

        StorageType storage;
        ReferenceState16 reference;

        srand(98765);

        for (size_t i = 0; i < c_stressIterationCount; ++i)
        {
            #pragma warning(suppress: 6328 6340) // size mismatch, sign mismatch
            LOG_OUTPUT(L"Iteration %d", i);

            auto targetField = GetRandomField<FieldEnum1>();

            switch (GetRandomAction())
            {
                case Action::Ensure:
                    EnsureAction(storage, targetField, reference);
                    break;

                case Action::Remove:
                    RemoveAction(storage, targetField, reference);
                    break;
            }

            ValidateValueField<FieldEnum1, FieldEnum1::field0>(storage, reference.f0);
            ValidateValueField<FieldEnum1, FieldEnum1::field1>(storage, reference.f1);
            ValidateValueField<FieldEnum1, FieldEnum1::field2>(storage, reference.f2);
            ValidateValueField<FieldEnum1, FieldEnum1::field3>(storage, reference.f3);
            ValidateSmartPtrField<FieldEnum1, FieldEnum1::field4>(storage, reference.f4);
            ValidateSmartPtrField<FieldEnum1, FieldEnum1::field5>(storage, reference.f5);
            ValidateSmartPtrField<FieldEnum1, FieldEnum1::field6>(storage, reference.f6);
            ValidateSmartPtrField<FieldEnum1, FieldEnum1::field7>(storage, reference.f7);
            ValidateValueField<FieldEnum1, FieldEnum1::field8>(storage, reference.f8);
            ValidateValueField<FieldEnum1, FieldEnum1::field9>(storage, reference.f9);
            ValidateValueField<FieldEnum1, FieldEnum1::field10>(storage, reference.f10);
            ValidateValueField<FieldEnum1, FieldEnum1::field11>(storage, reference.f11);
            ValidateCustomField<FieldEnum1, FieldEnum1::field12>(storage, reference.f12);
            ValidateCustomField<FieldEnum1, FieldEnum1::field13>(storage, reference.f13);
            ValidateValueField<FieldEnum1, FieldEnum1::field14>(storage, reference.f14);
            ValidateValueField<FieldEnum1, FieldEnum1::field15>(storage, reference.f15);
        }
    }
} } } } }
