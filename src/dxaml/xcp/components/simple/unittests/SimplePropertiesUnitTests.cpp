// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SimplePropertiesUnitTests.h"
#include "SimpleProperties.h"
#include <XamlLogging.h>
#include <CommonUtilities.h>

#define VERIFY_FAILFAST(__operation)                                                                                                                \
{                                                                                                                                                   \
    bool __exceptionHit = false;                                                                                                                    \
    try {                                                                                                                                           \
        __operation;                                                                                                                                \
    }                                                                                                                                               \
    catch(failfast_exception __e) {                                                                                                                 \
        __exceptionHit = true;                                                                                                                      \
        WEX::TestExecution::Private::MacroVerify::ExpectedExceptionThrown(__e, L"failfast_exception", L#__operation);                               \
    }                                                                                                                                               \
    if (!__exceptionHit) {                                                                                                                          \
        WEX::TestExecution::Private::MacroVerify::ExpectedExceptionNotThrown(L"failfast_exception", L#__operation, PRIVATE_VERIFY_ERROR_INFO);      \
    }                                                                                                                                               \
}

using namespace SimpleProperty;

extern sparsetables s_sparsetables;
extern changehandlerstables s_changehandlerstables;

SomeClass::~SomeClass()
{
    SimpleProperty::Property::NotifyDestroyed<SomeClass>(this);
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace SimpleProperties {

    bool SimplePropertiesUnitTests::ClassSetup()
    {
        return true;
    }

    bool SimplePropertiesUnitTests::ClassCleanup()
    {
        return true;
    }

    struct NotificationInfo
    {
        unsigned int id       = 0;
        objid_t obj           = {};
        const void* old_value = nullptr;

        void reset()
        {
            id = 0;
            obj = 0;
            old_value = nullptr;
        }
    };

    NotificationInfo lastNotification;

    template <TID tid>
    struct last_value
    {
        static type_from_tid<tid> value;
    };

    decltype(last_value<TID::Integer>::value) last_value<TID::Integer>::value = {};
    decltype(last_value<TID::BigStructOfChar>::value) last_value<TID::BigStructOfChar>::value = {};
    decltype(last_value<TID::BigStructOfInt>::value) last_value<TID::BigStructOfInt>::value = {};
    decltype(last_value<TID::SharedBigStructOfInt>::value) last_value<TID::SharedBigStructOfInt>::value = {};

    template <TID tid>
    void listen(objid_t obj, typename interface_types_from_tid<tid>::notify_value old_value)
    {
        lastNotification.id++;
        lastNotification.obj = obj;

        if (old_value)
        {
            last_value<tid>::value = *old_value;
            lastNotification.old_value = &last_value<tid>::value;
        }
        else
        {
            lastNotification.old_value = nullptr;
        }
    }

    // Registers property changed handlers for test properties.  If notifyOnUnchangedValue is true, the listen() callback
    // above will always be called when the property is set, even of the old and new values are the equal.
    static void RegisterHandlers(bool notifyOnUnchangedValue = false)
    {
        Property::id<PID::valueInField>::RegisterHandler(listen<TID::Integer>, notifyOnUnchangedValue);
        Property::id<PID::valueInSparse>::RegisterHandler(listen<TID::Integer>, notifyOnUnchangedValue);
        Property::id<PID::valueInGroupSparse>::RegisterHandler(listen<TID::Integer>, notifyOnUnchangedValue);

        Property::id<PID::smartInField>::RegisterHandler(listen<TID::SharedBigStructOfInt>, notifyOnUnchangedValue);
        Property::id<PID::smartInSparse>::RegisterHandler(listen<TID::SharedBigStructOfInt>, notifyOnUnchangedValue);
        Property::id<PID::smartInGroupSparse>::RegisterHandler(listen<TID::SharedBigStructOfInt>, notifyOnUnchangedValue);

        Property::id<PID::bigImmutableInField>::RegisterHandler(listen<TID::BigStructOfInt>, notifyOnUnchangedValue);
        Property::id<PID::bigImmutableInSparse>::RegisterHandler(listen<TID::BigStructOfInt>, notifyOnUnchangedValue);
        Property::id<PID::bigImmutableInGroupSparse>::RegisterHandler(listen<TID::BigStructOfInt>, notifyOnUnchangedValue);

        Property::id<PID::bigMutableInField>::RegisterHandler(listen<TID::BigStructOfChar>, notifyOnUnchangedValue);
        Property::id<PID::bigMutableInSparse>::RegisterHandler(listen<TID::BigStructOfChar>, notifyOnUnchangedValue);
        Property::id<PID::bigMutableInGroupSparse>::RegisterHandler(listen<TID::BigStructOfChar>, notifyOnUnchangedValue);
    }

    static void UnregisterHandlers()
    {
        Property::id<PID::valueInField>::UnregisterHandler(listen<TID::Integer>);
        Property::id<PID::valueInSparse>::UnregisterHandler(listen<TID::Integer>);
        Property::id<PID::valueInGroupSparse>::UnregisterHandler(listen<TID::Integer>);

        Property::id<PID::smartInField>::UnregisterHandler(listen<TID::SharedBigStructOfInt>);
        Property::id<PID::smartInSparse>::UnregisterHandler(listen<TID::SharedBigStructOfInt>);
        Property::id<PID::smartInGroupSparse>::UnregisterHandler(listen<TID::SharedBigStructOfInt>);

        Property::id<PID::bigImmutableInField>::UnregisterHandler(listen<TID::BigStructOfInt>);
        Property::id<PID::bigImmutableInSparse>::UnregisterHandler(listen<TID::BigStructOfInt>);
        Property::id<PID::bigImmutableInGroupSparse>::UnregisterHandler(listen<TID::BigStructOfInt>);

        Property::id<PID::bigMutableInField>::UnregisterHandler(listen<TID::BigStructOfChar>);
        Property::id<PID::bigMutableInSparse>::UnregisterHandler(listen<TID::BigStructOfChar>);
        Property::id<PID::bigMutableInGroupSparse>::UnregisterHandler(listen<TID::BigStructOfChar>);
    }

    template <Kind T> struct tag_kind : std::integral_constant<Kind, T>::type {};

    template <typename T, typename U>
    void ValidatePerKind(tag_kind<Kind::Value>, const T& value, const U& readback)
    {
    }

    template <typename T, typename U>
    void ValidatePerKind(tag_kind<Kind::ValueByRef>, const T& value, const U& readback)
    {
    }

    template <typename T, typename U>
    void ValidatePerKind(tag_kind<Kind::BigImmutable>, const T& value, const U& readback)
    {
        VERIFY_IS_TRUE(value.m_value == -1);        // it was moved out...
        VERIFY_IS_TRUE(readback.m_value != -1);     // into here...
    }

    template <typename T, typename U>
    void ValidatePerKind(tag_kind<Kind::BigMutable>, const T& value, const U& readback)
    {
        VERIFY_IS_TRUE(value.m_value == -1);        // it was moved out...
        VERIFY_IS_TRUE(readback.m_value != -1);     // into here...
    }

    template <PID pid, typename RT, typename T>
    void TestGetSetValue_CT(
        objid_t obj_id,
        T value)
    {
        const_objid_t cobj_id = obj_id;

        auto copy = value;
        Property::id<pid>::Set(obj_id, std::move(copy));
        RT readback = Property::id<pid>::Get(cobj_id);
        VERIFY_ARE_EQUAL(value, readback);
        VERIFY_IS_TRUE(Property::id<pid>::IsSet(cobj_id));
        ValidatePerKind(tag_kind<type_traits_from_pid<pid>::kind>(), copy, readback);
    }

    template <TID tid, typename RT, typename T>
    void TestGetSetValue_RT(
        objid_t obj_id,
        PID pid,
        T value)
    {
        const_objid_t cobj_id = obj_id;

        auto copy = value;
        Property::as<tid>::Set(obj_id, pid, std::move(copy));
        RT readback = Property::as<tid>::Get(cobj_id, pid);
        VERIFY_ARE_EQUAL(value, readback);
        VERIFY_IS_TRUE(Property::IsSet(cobj_id, pid));
        ValidatePerKind(tag_kind<type_traits<tid>::kind>(), copy, readback);
    }

    void SimplePropertiesUnitTests::GettersSetters()
    {
        SomeClass s1, s2;

        {
            VERIFY_IS_TRUE(Property::id<PID::valueInField>::IsSet(&s1));
            TestGetSetValue_CT<PID::valueInField, int>(&s1, 100);

            VERIFY_IS_TRUE(Property::IsSet(&s2, PID::valueInField));
            TestGetSetValue_RT<TID::Integer, int>(&s2, PID::valueInField, 200);
        }

        {
            VERIFY_IS_FALSE(Property::id<PID::valueInSparse>::IsSet(&s1));
            TestGetSetValue_CT<PID::valueInSparse, int>(&s1, 101);

            VERIFY_IS_FALSE(Property::IsSet(&s2, PID::valueInSparse));
            TestGetSetValue_RT<TID::Integer, int>(&s2, PID::valueInSparse, 201);
        }

        {
            VERIFY_IS_FALSE(Property::id<PID::valueInGroupSparse>::IsSet(&s1));
            TestGetSetValue_CT<PID::valueInGroupSparse, int>(&s1, 102);

            VERIFY_IS_FALSE(Property::IsSet(&s2, PID::valueInGroupSparse));
            TestGetSetValue_RT<TID::Integer, int>(&s2, PID::valueInGroupSparse, 202);
        }


        {
            VERIFY_IS_TRUE(Property::id<PID::smartInField>::IsSet(&s1));
            TestGetSetValue_CT<PID::smartInField, const std::shared_ptr<BigStruct<int>>&>(&s1, std::make_shared<BigStruct<int>>());

            VERIFY_IS_TRUE(Property::IsSet(&s2, PID::smartInField));
            TestGetSetValue_RT<TID::SharedBigStructOfInt, const std::shared_ptr<BigStruct<int>>&>(&s2, PID::smartInField, std::make_shared<BigStruct<int>>());
        }

        {
            VERIFY_IS_FALSE(Property::id<PID::smartInSparse>::IsSet(&s1));
            TestGetSetValue_CT<PID::smartInSparse, const std::shared_ptr<BigStruct<int>>&>(&s1, std::make_shared<BigStruct<int>>());

            VERIFY_IS_FALSE(Property::IsSet(&s2, PID::smartInSparse));
            TestGetSetValue_RT<TID::SharedBigStructOfInt, const std::shared_ptr<BigStruct<int>>&>(&s2, PID::smartInSparse, std::make_shared<BigStruct<int>>());
        }

        {
            VERIFY_IS_FALSE(Property::id<PID::smartInGroupSparse>::IsSet(&s1));
            TestGetSetValue_CT<PID::smartInGroupSparse, const std::shared_ptr<BigStruct<int>>&>(&s1, std::make_shared<BigStruct<int>>());

            VERIFY_IS_FALSE(Property::IsSet(&s2, PID::smartInGroupSparse));
            TestGetSetValue_RT<TID::SharedBigStructOfInt, const std::shared_ptr<BigStruct<int>>&>(&s2, PID::smartInGroupSparse, std::make_shared<BigStruct<int>>());
        }

        {
            VERIFY_IS_TRUE(Property::id<PID::bigImmutableInField>::IsSet(&s1));
            TestGetSetValue_CT<PID::bigImmutableInField, const BigStruct<int>&>(&s1, BigStruct<int>(13));

            VERIFY_IS_TRUE(Property::IsSet(&s2, PID::bigImmutableInField));
            TestGetSetValue_RT<TID::BigStructOfInt, const BigStruct<int>&>(&s2, PID::bigImmutableInField, BigStruct<int>(14));
        }

        {
            VERIFY_IS_FALSE(Property::id<PID::bigImmutableInSparse>::IsSet(&s1));
            TestGetSetValue_CT<PID::bigImmutableInSparse, const BigStruct<int>&>(&s1, BigStruct<int>(13));

            VERIFY_IS_FALSE(Property::IsSet(&s2, PID::bigImmutableInSparse));
            TestGetSetValue_RT<TID::BigStructOfInt, const BigStruct<int>&>(&s2, PID::bigImmutableInSparse, BigStruct<int>(14));
        }

        {
            VERIFY_IS_FALSE(Property::id<PID::bigImmutableInGroupSparse>::IsSet(&s1));
            TestGetSetValue_CT<PID::bigImmutableInGroupSparse, const BigStruct<int>&>(&s1, BigStruct<int>(13));

            VERIFY_IS_FALSE(Property::IsSet(&s2, PID::bigImmutableInGroupSparse));
            TestGetSetValue_RT<TID::BigStructOfInt, const BigStruct<int>&>(&s2, PID::bigImmutableInGroupSparse, BigStruct<int>(14));
        }

        {
            VERIFY_IS_TRUE(Property::id<PID::bigMutableInField>::IsSet(&s1));
            TestGetSetValue_CT<PID::bigMutableInField, BigStruct<char>&>(&s1, BigStruct<char>(13));

            VERIFY_IS_TRUE(Property::IsSet(&s2, PID::bigMutableInField));
            TestGetSetValue_RT<TID::BigStructOfChar, BigStruct<char>&>(&s2, PID::bigMutableInField, BigStruct<char>(14));
        }

        {
            VERIFY_IS_FALSE(Property::id<PID::bigMutableInSparse>::IsSet(&s1));
            TestGetSetValue_CT<PID::bigMutableInSparse, BigStruct<char>&>(&s1, BigStruct<char>(13));

            VERIFY_IS_FALSE(Property::IsSet(&s2, PID::bigMutableInSparse));
            TestGetSetValue_RT<TID::BigStructOfChar, BigStruct<char>&>(&s2, PID::bigMutableInSparse, BigStruct<char>(14));
        }

        {
            VERIFY_IS_FALSE(Property::id<PID::bigMutableInGroupSparse>::IsSet(&s1));
            TestGetSetValue_CT<PID::bigMutableInGroupSparse, BigStruct<char>&>(&s1, BigStruct<char>(13));

            VERIFY_IS_FALSE(Property::IsSet(&s2, PID::bigMutableInGroupSparse));
            TestGetSetValue_RT<TID::BigStructOfChar, BigStruct<char>&>(&s2, PID::bigMutableInGroupSparse, BigStruct<char>(14));
        }
    }

    void SimplePropertiesUnitTests::GetUnset()
    {
        SomeClass s1;

        VERIFY_NO_THROW(Property::id<PID::valueInField>::Get(&s1));
        VERIFY_NO_THROW(Property::id<PID::valueInSparse>::Get(&s1));
        VERIFY_NO_THROW(Property::id<PID::valueInGroupSparse>::Get(&s1));

        VERIFY_NO_THROW(Property::id<PID::smartInField>::Get(&s1));
        VERIFY_NO_THROW(Property::id<PID::smartInSparse>::Get(&s1));
        VERIFY_NO_THROW(Property::id<PID::smartInGroupSparse>::Get(&s1));

        VERIFY_NO_THROW(Property::id<PID::bigImmutableInField>::Get(&s1));
        VERIFY_NO_THROW(Property::id<PID::bigImmutableInSparse>::Get(&s1));
        VERIFY_NO_THROW(Property::id<PID::bigImmutableInGroupSparse>::Get(&s1));

        VERIFY_NO_THROW(Property::id<PID::bigMutableInField>::Get(&s1));

        VERIFY_FAILFAST(Property::id<PID::bigMutableInSparse>::Get(&s1));
        VERIFY_FAILFAST(Property::id<PID::bigMutableInGroupSparse>::Get(&s1));
    }

    void SimplePropertiesUnitTests::SetToDefaultOnFirstSet()
    {
        SomeClass s1;
        lastNotification.id = 0;

        RegisterHandlers();

        Property::id<PID::valueInField>::Set(&s1, Property::Default<PID::valueInField>());
        VERIFY_IS_TRUE(lastNotification.id == 0);

        Property::id<PID::valueInSparse>::Set(&s1, Property::Default<PID::valueInSparse>());
        VERIFY_IS_TRUE(lastNotification.id == 0);

        Property::id<PID::valueInGroupSparse>::Set(&s1, Property::Default<PID::valueInGroupSparse>());
        VERIFY_IS_TRUE(lastNotification.id == 0);


        Property::id<PID::smartInField>::Set(&s1, Property::Default<PID::smartInField>());
        VERIFY_IS_TRUE(lastNotification.id == 0);

        Property::id<PID::smartInSparse>::Set(&s1, Property::Default<PID::smartInSparse>());
        VERIFY_IS_TRUE(lastNotification.id == 0);

        Property::id<PID::smartInGroupSparse>::Set(&s1, Property::Default<PID::smartInGroupSparse>());
        VERIFY_IS_TRUE(lastNotification.id == 0);


        {
            auto tempDefault = Property::Default<PID::bigImmutableInField>();
            Property::id<PID::bigImmutableInField>::Set(&s1, std::move(tempDefault));
            VERIFY_IS_TRUE(lastNotification.id == 0);
        }

        {
            auto tempDefault = Property::Default<PID::bigImmutableInSparse>();
            Property::id<PID::bigImmutableInSparse>::Set(&s1, std::move(tempDefault));
            VERIFY_IS_TRUE(lastNotification.id == 0);
        }

        {
            auto tempDefault = Property::Default<PID::bigImmutableInGroupSparse>();
            Property::id<PID::bigImmutableInGroupSparse>::Set(&s1, std::move(tempDefault));
            VERIFY_IS_TRUE(lastNotification.id == 0);
        }


        Property::id<PID::bigMutableInField>::Set(&s1, std::move(BigStruct<char>()));
        VERIFY_IS_TRUE(lastNotification.id == 0);

        Property::id<PID::bigMutableInSparse>::Set(&s1, std::move(BigStruct<char>()));
        VERIFY_IS_TRUE(lastNotification.id == 1);

        Property::id<PID::bigMutableInGroupSparse>::Set(&s1, std::move(BigStruct<char>()));
        VERIFY_IS_TRUE(lastNotification.id == 2);

        UnregisterHandlers();
    }

    template <PID pid, typename T>
    static void SetToDefaultAfterNonDefaultSetScenario(
        objid_t obj,
        T value)
    {
        lastNotification.id = 0;
        Property::id<pid>::Set(obj, std::move(value));
        auto tempDefault = Property::Default<pid>();
        Property::id<pid>::Set(obj, std::move(tempDefault));
        VERIFY_IS_TRUE(lastNotification.id == 2);
        VERIFY_ARE_EQUAL(Property::Default<pid>(), Property::id<pid>::Get(obj));
        VERIFY_IS_TRUE(Property::id<pid>::IsSet(obj));
    }

    void SimplePropertiesUnitTests::SetToDefaultAfterNonDefaultSet()
    {
        SomeClass s1;

        RegisterHandlers();

        SetToDefaultAfterNonDefaultSetScenario<PID::valueInField>(&s1, 13);
        SetToDefaultAfterNonDefaultSetScenario<PID::valueInSparse>(&s1, 13);
        SetToDefaultAfterNonDefaultSetScenario<PID::valueInGroupSparse>(&s1, 13);

        SetToDefaultAfterNonDefaultSetScenario<PID::smartInField>(&s1, std::make_shared<BigStruct<int>>(101));
        SetToDefaultAfterNonDefaultSetScenario<PID::smartInSparse>(&s1, std::make_shared<BigStruct<int>>(101));
        SetToDefaultAfterNonDefaultSetScenario<PID::smartInGroupSparse>(&s1, std::make_shared<BigStruct<int>>(101));

        SetToDefaultAfterNonDefaultSetScenario<PID::bigImmutableInField>(&s1, std::move(BigStruct<int>(101)));
        SetToDefaultAfterNonDefaultSetScenario<PID::bigImmutableInSparse>(&s1, std::move(BigStruct<int>(101)));
        SetToDefaultAfterNonDefaultSetScenario<PID::bigImmutableInGroupSparse>(&s1, std::move(BigStruct<int>(101)));

        // PID::bigMutableIn* not applicable, since there's no default.

        UnregisterHandlers();
    }

    template <PID pid, typename T>
    static void MultipleSetsValue(
        objid_t obj,
        T value1,
        T value2)
    {
        using prop_traits = prop_traits<pid>;

        lastNotification.reset();

        auto value1_copy = value1;
        Property::id<pid>::Set(obj, std::move(value1_copy));
        VERIFY_IS_TRUE(lastNotification.id == 1);
        VERIFY_ARE_EQUAL(lastNotification.obj, obj);

        if constexpr (prop_traits::storage == StorageType::Field)
        {
            VERIFY_IS_NOT_NULL(lastNotification.old_value);
            VERIFY_ARE_EQUAL(&last_value<prop_traits::tid>::value, lastNotification.old_value);
        }
        else
        {
            VERIFY_IS_NULL(lastNotification.old_value);
        }
        auto&& readback1 = Property::id<pid>::Get(obj);
        VERIFY_ARE_EQUAL(readback1, value1);

        value1_copy = value1;
        Property::id<pid>::Set(obj, std::move(value1_copy));
        VERIFY_IS_TRUE(lastNotification.id == 1);
        auto&& readback2 = Property::id<pid>::Get(obj);
        VERIFY_ARE_EQUAL(readback2, value1);

        auto value2_copy = value2;
        Property::id<pid>::Set(obj, std::move(value2_copy));
        VERIFY_IS_TRUE(lastNotification.id == 2);
        VERIFY_ARE_EQUAL(lastNotification.obj, obj);
        VERIFY_IS_NOT_NULL(lastNotification.old_value);
        VERIFY_ARE_EQUAL(&last_value<prop_traits::tid>::value, lastNotification.old_value);
        VERIFY_ARE_EQUAL(last_value<prop_traits::tid>::value, value1);
        auto&& readback3 = Property::id<pid>::Get(obj);
        VERIFY_ARE_EQUAL(readback3, value2);
    }

    void SimplePropertiesUnitTests::MultipleSets()
    {
        RegisterHandlers();

        {
            SomeClass s1;
            MultipleSetsValue<PID::valueInField>(&s1, 230, 231);
            MultipleSetsValue<PID::valueInSparse>(&s1, 232, 233);
            MultipleSetsValue<PID::valueInGroupSparse>(&s1, 234, 235);
        }

        {
            SomeClass s1;

            MultipleSetsValue<PID::smartInField>(&s1, std::make_shared<BigStruct<int>>(101), std::make_shared<BigStruct<int>>(102));
            MultipleSetsValue<PID::smartInSparse>(&s1, std::make_shared<BigStruct<int>>(103), std::make_shared<BigStruct<int>>(104));
            MultipleSetsValue<PID::smartInGroupSparse>(&s1, std::make_shared<BigStruct<int>>(105), std::make_shared<BigStruct<int>>(106));
        }

        {
            SomeClass s1;

            MultipleSetsValue<PID::bigImmutableInField>(&s1, BigStruct<int>(101), BigStruct<int>(102));
            MultipleSetsValue<PID::bigImmutableInSparse>(&s1, BigStruct<int>(103), BigStruct<int>(104));
            MultipleSetsValue<PID::bigImmutableInGroupSparse>(&s1, BigStruct<int>(105), BigStruct<int>(106));
        }

        {
            SomeClass s1;

            MultipleSetsValue<PID::bigMutableInField>(&s1, BigStruct<char>(101), BigStruct<char>(102));
            MultipleSetsValue<PID::bigMutableInSparse>(&s1, BigStruct<char>(103), BigStruct<char>(104));
            MultipleSetsValue<PID::bigMutableInGroupSparse>(&s1, BigStruct<char>(105), BigStruct<char>(106));
        }

        UnregisterHandlers();
    }

    void SimplePropertiesUnitTests::RT_TypeMismatchThrows()
    {
        SomeClass s1;

        VERIFY_FAILFAST(Property::as<TID::Integer>::Get(&s1, PID::smartInField));
        VERIFY_FAILFAST(Property::as<TID::Integer>::Set(&s1, PID::smartInField, 99));
        VERIFY_FAILFAST(Property::as<TID::Integer>::RegisterHandler(PID::smartInField, listen<TID::Integer>));
    }

    void SimplePropertiesUnitTests::LifetimeControlledByHost()
    {
        objid_t obj_id = nullptr;

        {
            SomeClass s1;

            obj_id = &s1;

            Property::id<PID::valueInSparse>::Set(obj_id, 10);
            Property::id<PID::valueInGroupSparse>::Set(obj_id, 11);
            Property::id<PID::smartInSparse>::Set(obj_id, std::make_shared<BigStruct<int>>(102));
            Property::id<PID::smartInGroupSparse>::Set(obj_id, std::make_shared<BigStruct<int>>(102));
            Property::id<PID::bigImmutableInSparse>::Set(obj_id, std::move(BigStruct<int>(105)));
            Property::id<PID::bigImmutableInGroupSparse>::Set(obj_id, std::move(BigStruct<int>(105)));
            Property::id<PID::bigMutableInSparse>::Set(obj_id, std::move(BigStruct<char>(105)));
            Property::id<PID::bigMutableInGroupSparse>::Set(obj_id, std::move(BigStruct<char>(105)));
        }

        VERIFY_IS_TRUE(s_sparsetables.m_valueInSparse.size() == 0);
        VERIFY_IS_TRUE(s_sparsetables.m_smartInSparse.size() == 0);
        VERIFY_IS_TRUE(s_sparsetables.m_bigImmutableInSparse.size() == 0);
        VERIFY_IS_TRUE(s_sparsetables.m_bigMutableInSparse.size() == 0);
        VERIFY_IS_TRUE(s_sparsetables.m_groupStorage.size() == 0);
    }

    void SimplePropertiesUnitTests::RegistrationRefCounting()
    {
        VERIFY_IS_TRUE(s_changehandlerstables.m_valueInField.size() == 0);
        Property::id<PID::valueInField>::RegisterHandler(listen<TID::Integer>);
        VERIFY_IS_TRUE(s_changehandlerstables.m_valueInField.size() == 1);
        Property::id<PID::valueInField>::RegisterHandler(listen<TID::Integer>);
        VERIFY_IS_TRUE(s_changehandlerstables.m_valueInField.size() == 1);
        Property::id<PID::valueInField>::UnregisterHandler(listen<TID::Integer>);
        VERIFY_IS_TRUE(s_changehandlerstables.m_valueInField.size() == 1);
        Property::id<PID::valueInField>::UnregisterHandler(listen<TID::Integer>);
        VERIFY_IS_TRUE(s_changehandlerstables.m_valueInField.size() == 0);
    }

    template <PID pid, typename T>
    static void MultipleSetsValueAlwaysChange(
        objid_t obj,
        T value1,
        T value2,
        bool shouldAlwaysChange)
    {
        using prop_traits = prop_traits<pid>;

        // Ensure our property has value1 before any testing.
        auto value1_copy = value1;
        Property::id<pid>::Set(obj, std::move(value1_copy));
        lastNotification.reset();

        // Set the same value1 again.  Verify we got a notification if we should get notifications
        // when the same value is reapplied.  If we were supposed to get a notification, verify
        // the information is correct
        value1_copy = value1;
        Property::id<pid>::Set(obj, std::move(value1_copy));
        VERIFY_ARE_EQUAL(lastNotification.id, shouldAlwaysChange ? 1 : 0);

        if (shouldAlwaysChange)
        {
            VERIFY_ARE_EQUAL(lastNotification.obj, obj);
            VERIFY_IS_NOT_NULL(lastNotification.old_value);
            VERIFY_ARE_EQUAL(&last_value<prop_traits::tid>::value, lastNotification.old_value);
        }

        // Verify the property really was set to value1.
        auto&& readback1 = Property::id<pid>::Get(obj);
        VERIFY_ARE_EQUAL(readback1, value1);

        // Try setting value2, and verify the property value is now value2.
        auto value2_copy = value2;
        Property::id<pid>::Set(obj, std::move(value2_copy));
        VERIFY_ARE_EQUAL(lastNotification.id, shouldAlwaysChange ? 2 : 1);
        auto&& readback2 = Property::id<pid>::Get(obj);
        VERIFY_ARE_EQUAL(readback2, value2);
    }

    template <PID pid, typename T>
    static void MultipleSetsValueAlwaysChange(
        objid_t obj,
        T value1,
        bool shouldAlwaysChange)
    {
        auto default_value = Property::Default<pid>();
        MultipleSetsValueAlwaysChange<pid>(obj, value1, shouldAlwaysChange, default_value);
    }

    static void ChangeNotificationsAlwaysNotifyAllProperties(bool shouldAlwaysChange)
    {
        {
            SomeClass s1;
            MultipleSetsValueAlwaysChange<PID::valueInField>(&s1, 230, 231, shouldAlwaysChange);
            MultipleSetsValueAlwaysChange<PID::valueInSparse>(&s1, 232, 233, shouldAlwaysChange);
            MultipleSetsValueAlwaysChange<PID::valueInGroupSparse>(&s1, 234, 235, shouldAlwaysChange);
        }

        {
            SomeClass s1;

            MultipleSetsValueAlwaysChange<PID::smartInField>(&s1, std::make_shared<BigStruct<int>>(101), std::make_shared<BigStruct<int>>(102), shouldAlwaysChange);
            MultipleSetsValueAlwaysChange<PID::smartInSparse>(&s1, std::make_shared<BigStruct<int>>(103), std::make_shared<BigStruct<int>>(104), shouldAlwaysChange);
            MultipleSetsValueAlwaysChange<PID::smartInGroupSparse>(&s1, std::make_shared<BigStruct<int>>(105), std::make_shared<BigStruct<int>>(106), shouldAlwaysChange);
        }

        {
            SomeClass s1;

            MultipleSetsValueAlwaysChange<PID::bigImmutableInField>(&s1, BigStruct<int>(101), BigStruct<int>(102), shouldAlwaysChange);
            MultipleSetsValueAlwaysChange<PID::bigImmutableInSparse>(&s1, BigStruct<int>(103), BigStruct<int>(104), shouldAlwaysChange);
            MultipleSetsValueAlwaysChange<PID::bigImmutableInGroupSparse>(&s1, BigStruct<int>(105), BigStruct<int>(106), shouldAlwaysChange);
        }

        {
            SomeClass s1;

            MultipleSetsValueAlwaysChange<PID::bigMutableInField>(&s1, BigStruct<char>(101), BigStruct<char>(102), shouldAlwaysChange);
            MultipleSetsValueAlwaysChange<PID::bigMutableInSparse>(&s1, BigStruct<char>(103), BigStruct<char>(104), shouldAlwaysChange);
            MultipleSetsValueAlwaysChange<PID::bigMutableInGroupSparse>(&s1, BigStruct<char>(105), BigStruct<char>(106), shouldAlwaysChange);
        }
    }

    void SimplePropertiesUnitTests::ChangeNotificationsAlwaysNotify()
    {
        // Register handlers which always receive notifications, even when the property hasn't changed
        // Do some property sets using the same value to ensure we get notifications, even though
        // the value didn't change
        RegisterHandlers(true);

        ChangeNotificationsAlwaysNotifyAllProperties(true);

        UnregisterHandlers();

        // Reregister with handlers that don't get notifications when the property value is the same,
        // and ensure they don't receive notifications for the same value
        RegisterHandlers(false);

        ChangeNotificationsAlwaysNotifyAllProperties(false);

        UnregisterHandlers();
    }
} } } } }
