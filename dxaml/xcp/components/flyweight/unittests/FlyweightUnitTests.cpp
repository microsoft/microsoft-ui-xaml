// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FlyweightUnitTests.h"
#include "FlyweightFactory.h"
#include <XamlLogging.h>
#include <cstdlib>
#include <memory>
#include <CommonUtilities.h>

using namespace WEX::Common;

struct TestVO
{
    using Wrapper = ::Flyweight::ValueObjectWrapper<TestVO>;

    TestVO() = delete;

    TestVO(
        float floatValue,
        int intValue,
        void* ptrValue)
        : m_floatValue(floatValue)
        , m_intValue(intValue)
        , m_ptrValue(ptrValue)
    {}

    float m_floatValue;
    int m_intValue;
    void* m_ptrValue;
};

struct TestVO2
{
    using Wrapper = ::Flyweight::ValueObjectWrapper<TestVO2>;

    TestVO2() = delete;

    TestVO2(
        int intValue)
        : m_intValue(intValue)
    {}

    int m_intValue;
};

namespace Flyweight
{
    namespace Operators
    {
        template <>
        inline TestVO Default()
        {
            return TestVO(
                       0.0f,
                       0,
                       nullptr);
        }

        template <>
        inline bool equal(const TestVO& lhs, const TestVO& rhs)
        {
            return std::tie(lhs.m_floatValue, lhs.m_intValue, lhs.m_ptrValue) ==
                   std::tie(rhs.m_floatValue, rhs.m_intValue, rhs.m_ptrValue);
        }

        template <>
        inline bool less(const TestVO& lhs, const TestVO& rhs)
        {
            return std::tie(lhs.m_floatValue, lhs.m_intValue, lhs.m_ptrValue) <
                   std::tie(rhs.m_floatValue, rhs.m_intValue, rhs.m_ptrValue);
        }

        template <>
        inline std::size_t hash(const TestVO& inst)
        {
            std::size_t hash = 0;
            CommonUtilities::hash_combine(hash, inst.m_floatValue);
            CommonUtilities::hash_combine(hash, inst.m_intValue);
            CommonUtilities::hash_combine(hash, inst.m_ptrValue);
            return hash;
        }

        template <>
        inline TestVO2 Default()
        {
            return TestVO2(0);
        }

        template <>
        inline bool equal(const TestVO2& lhs, const TestVO2& rhs)
        {
            return std::tie(lhs.m_intValue) ==
                   std::tie(rhs.m_intValue);
        }

        template <>
        inline bool less(const TestVO2& lhs, const TestVO2& rhs)
        {
            return std::tie(lhs.m_intValue) <
                   std::tie(rhs.m_intValue);
        }

        template <>
        inline std::size_t hash(const TestVO2& inst)
        {
            return 0;
        }
    }
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Flyweight {

    bool FlyweightUnitTests::ClassSetup()
    {
        return true;
    }

    void FlyweightUnitTests::BasicTests()
    {
        ::Flyweight::Factory<TestVO>::State state;

        // Get default VO.

        xref_ptr<TestVO::Wrapper> ptr_default = ::Flyweight::Factory<TestVO>::Create(
            &state,
            0.0f,
            0,
            nullptr);

        VERIFY_ARE_EQUAL(0.0f, ptr_default->Value().m_floatValue);
        VERIFY_ARE_EQUAL(0, ptr_default->Value().m_intValue);
        VERIFY_ARE_EQUAL(nullptr, ptr_default->Value().m_ptrValue);
        VERIFY_ARE_EQUAL(2, ptr_default->GetRefCount());

        // Get a non-default VO.

        xref_ptr<TestVO::Wrapper> ptr_0 = ::Flyweight::Factory<TestVO>::Create(
            &state,
            3.1415f,
            13,
            ptr_default.get());

        VERIFY_ARE_EQUAL(3.1415f, ptr_0->Value().m_floatValue);
        VERIFY_ARE_EQUAL(13, ptr_0->Value().m_intValue);
        VERIFY_ARE_EQUAL(ptr_default.get(), ptr_0->Value().m_ptrValue);
        VERIFY_ARE_EQUAL(2, ptr_0->GetRefCount());

        // Get a different non-default VO.

        xref_ptr<TestVO::Wrapper> ptr_1 = ::Flyweight::Factory<TestVO>::Create(
            &state,
            0.99f,
            132,
            ptr_default.get());

        VERIFY_ARE_EQUAL(0.99f, ptr_1->Value().m_floatValue);
        VERIFY_ARE_EQUAL(132, ptr_1->Value().m_intValue);
        VERIFY_ARE_EQUAL(ptr_default.get(), ptr_1->Value().m_ptrValue);
        VERIFY_ARE_EQUAL(2, ptr_1->GetRefCount());

        // Get one which should already exist.

        xref_ptr<TestVO::Wrapper> ptr_01 = ::Flyweight::Factory<TestVO>::Create(
            &state,
            3.1415f,
            13,
            ptr_default.get());

        VERIFY_ARE_EQUAL(3.1415f, ptr_01->Value().m_floatValue);
        VERIFY_ARE_EQUAL(13, ptr_01->Value().m_intValue);
        VERIFY_ARE_EQUAL(ptr_default.get(), ptr_01->Value().m_ptrValue);
        VERIFY_ARE_EQUAL(ptr_0.get(), ptr_01.get());
        VERIFY_ARE_EQUAL(3, ptr_01->GetRefCount());
    }

    void FlyweightUnitTests::OverflowTests()
    {
        ::Flyweight::Factory<TestVO2>::State state;

        std::vector<xref_ptr<TestVO2::Wrapper>> ptrs;

        // Add unique VOs to max capacity.

        for (int i = 0; i < 271; ++i)
        {
            ptrs.emplace_back(
                ::Flyweight::Factory<TestVO2>::Create(
                    &state,
                    i));
        }

        // All should be managed by factory...

        for (const auto& ptr : ptrs)
        {
            VERIFY_ARE_EQUAL(2, ptr->GetRefCount());
        }

        // Adding past capacity will trigger pass-through.

        xref_ptr<TestVO2::Wrapper> ptr1 = ::Flyweight::Factory<TestVO2>::Create(
            &state,
            99999);

        VERIFY_ARE_EQUAL(1, ptr1->GetRefCount());

        // Drop all references to existing ones and trigger purge by going over min purge limit.
        // Factory should have no VOs after this.

        ptrs.clear();

        for (int i = 0; i < 49; ++i)
        {
            xref_ptr<TestVO2::Wrapper> temp = ::Flyweight::Factory<TestVO2>::Create(
                &state,
                999);
        }

        // Adding a new one, which should be managed by factory.

        xref_ptr<TestVO2::Wrapper> ptr2 = ::Flyweight::Factory<TestVO2>::Create(
            &state,
            99999);

        VERIFY_ARE_EQUAL(2, ptr2->GetRefCount());

        // Even though the state is the same, they are not the same instances.

        VERIFY_IS_TRUE(::Flyweight::Operators::equal(ptr1->Value(), ptr2->Value()));
        VERIFY_ARE_NOT_EQUAL(ptr1.get(), ptr2.get());
    }
} } } } }
