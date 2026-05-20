// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CValueUnitTests.h"

#include <MetadataAPI.h>
#include <TypeTableStructs.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CDependencyObject.h>
#include <primitives.h>
#include <CValue.h>
#include <CValueTypeInfo.h>
#include <CValue\unittests\ValueBoxer.h>
#include <CValueUtil.h>
#include "CommonUtilities.h"

#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

using namespace CValueDetails;

enum class Kind
{
    Unknown,
    Empty,
    Value,
    Ref,
    RefCount,
    Array,
    String
};

template <ValueType valueType, typename = void>
struct selector
{};

template <ValueType valueType>
struct selector<
    valueType,
    typename std::enable_if<
    valueType == valueAny ||
    valueType == valueNull>::type>
{
    static constexpr Kind kind = Kind::Empty;
};

template <ValueType valueType>
struct selector<
    valueType,
    typename std::enable_if<
    valueType == valueBool ||
    valueType == valueEnum ||
    valueType == valueEnum8 ||
    valueType == valueSigned ||
    valueType == valueUnsigned ||
    valueType == valueInt64 ||
    valueType == valueUInt64 ||
    valueType == valueFloat ||
    valueType == valueDouble ||
    valueType == valueColor ||
    valueType == valueDateTime ||
    valueType == valueTimeSpan ||
    valueType == valueInternalHandler ||
    valueType == valueTypeHandle ||
    valueType == valuePointer ||
    valueType == valueTextRange>::type>
{
    static constexpr Kind kind = Kind::Value;
};

template <ValueType valueType>
struct selector<
    valueType,
    typename std::enable_if<
    valueType == valuePoint ||
    valueType == valueSize ||
    valueType == valueRect ||
    valueType == valueThickness ||
    valueType == valueGridLength ||
    valueType == valueCornerRadius>::type>
{
    static constexpr Kind kind = Kind::Ref;
};

template <ValueType valueType>
struct selector<
    valueType,
    typename std::enable_if<
    valueType == valueObject ||
    valueType == valueVO ||
    valueType == valueIUnknown ||
    valueType == valueIInspectable ||
    valueType == valueThemeResource>::type>
{
    static constexpr Kind kind = Kind::RefCount;
};

template <ValueType valueType>
struct selector<
    valueType,
    typename std::enable_if<
    valueType == valueSignedArray ||
    valueType == valueFloatArray ||
    valueType == valueDoubleArray ||
    valueType == valuePointArray>::type>
{
    static constexpr Kind kind = Kind::Array;
};

template <ValueType valueType>
struct selector<
    valueType,
    typename std::enable_if<
    valueType == valueString>::type>
{
    static constexpr Kind kind = Kind::String;
};

template <Kind kind>
struct tag_selector {};

HRESULT __stdcall DummyEventHandler(CDependencyObject*, CEventArgs*)
{
    return S_OK;
}

struct TestVO
{
    using Wrapper = ::Flyweight::PropertyValueObjectWrapper<TestVO>;
    static constexpr KnownTypeIndex s_typeIndex = static_cast<KnownTypeIndex>(0);

    TestVO() = delete;

    TestVO(
        float floatValue)
        : m_floatValue(floatValue)
    {}

    float m_floatValue;
};

namespace Flyweight
{
    template <>
    typename Factory<TestVO>::State* GetFactoryState<TestVO>(CCoreServices* core)
    {
        return nullptr;
    }

    namespace Operators
    {
        template <>
        inline TestVO Default()
        {
            return TestVO(
                       0.0f);
        }

        template <>
        inline bool equal(const TestVO& lhs, const TestVO& rhs)
        {
            return std::tie(lhs.m_floatValue) ==
                   std::tie(rhs.m_floatValue);
        }

        template <>
        inline bool less(const TestVO& lhs, const TestVO& rhs)
        {
            return std::tie(lhs.m_floatValue) <
                   std::tie(rhs.m_floatValue);
        }

        template <>
        inline std::size_t hash(const TestVO& inst)
        {
            std::size_t hash = 0;
            CommonUtilities::hash_combine(hash, inst.m_floatValue);
            return hash;
        }
    }
}

namespace Windows
{
    namespace UI
    {
        namespace Xaml
        {
            namespace Tests
            {
                namespace CValue
                {
                    bool CValueUnitTests::ClassSetup()
                    {
                        HRESULT hr = ::CoInitialize(nullptr);
                        return (hr == S_OK);
                    }

                    bool CValueUnitTests::ClassCleanup()
                    {
                        ::CoUninitialize();
                        return true;
                    }

                    // Helpers

                    static void SetValue(ValueType valueType, ::CValue& value, bool empty)
                    {
                        switch (valueType)
                        {
                            case valueAny:
                                value.Unset();
                                break;

                            case valueNull:
                                value.SetNull();
                                break;

                            case valueBool:
                                value.Set<valueBool>(!empty);
                                break;

                            case valueEnum:
                                value.SetEnum(!empty * valueEnum);
                                break;

                            case valueEnum8:
                                // set the value to valueEnum so that we can compare against valueEnums correctly
                                value.SetEnum8(!empty * valueEnum); 
                                break;

                            case valueSigned:
                                value.Set<valueSigned>(!empty * valueSigned);
                                break;

                            case valueUnsigned:
                                value.Set<valueUnsigned>(!empty * valueUnsigned);
                                break;

                            case valueInt64:
                                value.Set<valueInt64>(!empty * valueInt64);
                                break;

                            case valueUInt64:
                                value.Set<valueUInt64>(!empty * valueUInt64);
                                break;

                            case valueFloat:
                                value.Set<valueFloat>((float)(!empty * valueFloat));
                                break;

                            case valueDouble:
                                value.Set<valueDouble>(!empty * valueDouble);
                                break;

                            case valueSignedArray:
                                if (empty)
                                {
                                    value.SetArray<valueSignedArray>(0, nullptr);
                                }
                                else
                                {
                                    value.SetArray<valueSignedArray>(3, new int32_t[3]{ 3, 4, 5 });
                                }
                                break;

                            case valueFloatArray:
                                if (empty)
                                {
                                    value.SetArray<valueFloatArray>(0, nullptr);
                                }
                                else
                                {
                                    value.SetArray<valueFloatArray>(3, new float[3]{ 6.f, 7.f, 8.f });
                                }
                                break;

                            case valueDoubleArray:
                                if (empty)
                                {
                                    value.SetArray<valueDoubleArray>(0, nullptr);
                                }
                                else
                                {
                                    value.SetArray<valueDoubleArray>(3, new double[3]{ 9, 10, 11 });
                                }
                                break;

                            case valuePointArray:
                                if (empty)
                                {
                                    value.SetArray<valuePointArray>(0, nullptr);
                                }
                                else
                                {
                                    value.SetArray<valuePointArray>(3, new XPOINTF[3]{ { 3.f, 4.f },{ 5.f, 6.f },{ 7.f, 8.f } });
                                }
                                break;

                            case valueString:
                                {
                                    if (empty)
                                    {
                                        value.SetString(xstring_ptr::NullString());
                                    }
                                    else
                                    {
                                        wrl_wrappers::HString str;
                                        str.Set(L"ABC");
                                        VERIFY_SUCCEEDED(value.SetString(str));
                                    }
                                }
                                break;

                            case valueColor:
                                value.Set<valueColor>(!empty * valueColor);
                                break;

                            case valuePoint:
                                if (empty)
                                {
                                    value.Set<valuePoint>(nullptr);
                                }
                                else
                                {
                                    value.Set<valuePoint>(new XPOINTF{ valuePoint });
                                }
                                break;

                            case valueSize:
                                if (empty)
                                {
                                    value.Set<valueSize>(nullptr);
                                }
                                else
                                {
                                    value.Set<valueSize>(new XSIZEF{ valueSize });
                                }
                                break;

                            case valueRect:
                                if (empty)
                                {
                                    value.Set<valueRect>(nullptr);
                                }
                                else
                                {
                                    value.Set<valueRect>(new XRECTF{ valueRect });
                                }
                                break;

                            case valueThickness:
                                if (empty)
                                {
                                    value.Set<valueThickness>(nullptr);
                                }
                                else
                                {
                                    value.Set<valueThickness>(new XTHICKNESS{ valueThickness });
                                }
                                break;

                            case valueGridLength:
                                if (empty)
                                {
                                    value.Set<valueGridLength>(nullptr);
                                }
                                else
                                {
                                    value.Set<valueGridLength>(new XGRIDLENGTH{ static_cast<DirectUI::GridUnitType>(valueGridLength), 0 });
                                }
                                break;

                            case valueCornerRadius:
                                if (empty)
                                {
                                    value.Set<valueCornerRadius>(nullptr);
                                }
                                else
                                {
                                    value.Set<valueCornerRadius>(new XCORNERRADIUS{ valueCornerRadius });
                                }
                                break;

                            case valueDateTime:
                                value.Set<valueDateTime>(wf::DateTime{ !empty * valueDateTime });
                                break;

                            case valueTimeSpan:
                                value.Set<valueTimeSpan>(wf::TimeSpan{ !empty * valueTimeSpan });
                                break;

                            case valueObject:
                                if (empty)
                                {
                                    value.Set<valueObject>(nullptr);
                                }
                                else
                                {
                                    value.Set<valueObject>(new CDependencyObject());
                                }
                                break;

                            case valueVO:
                                if (empty)
                                {
                                    value.Set<valueVO>(nullptr);
                                }
                                else
                                {
                                    value.Set<valueVO>(::Flyweight::Create<::TestVO>(nullptr, 2016.03f).detach());
                                }
                                break;

                            case valueInternalHandler:
                                if (empty)
                                {
                                    value.Set<valueInternalHandler>(nullptr);
                                }
                                else
                                {
                                    value.Set<valueInternalHandler>(&::DummyEventHandler);
                                }
                                break;

                            case valueIUnknown:
                                if (empty)
                                {
                                    value.Set<valueIUnknown>(nullptr);
                                }
                                else
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateSize({ 13.5f, 4.6f }, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIUnknown>(static_cast<IUnknown*>(spValue.Detach()));
                                }
                                break;

                            case valueIInspectable:
                                if (empty)
                                {
                                    value.Set<valueIInspectable>(nullptr);
                                }
                                else
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreatePoint({ valuePoint }, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueTypeHandle:
                                value.Set<valueTypeHandle>(empty ? KnownTypeIndex::UnknownType : KnownTypeIndex::AnimationDirection);
                                break;

                            case valueThemeResource:
                                if (empty)
                                {
                                    value.Wrap<valueThemeResource>(nullptr);
                                }
                                else
                                {
                                    value.Wrap<valueThemeResource>(reinterpret_cast<CThemeResource*>(1));
                                }
                                break;

                            case valuePointer:
                                if (empty)
                                {
                                    value.Set<valuePointer>(nullptr);
                                }
                                else
                                {
                                    value.Set<valuePointer>(&::DummyEventHandler);
                                }
                                break;

                            case valueTextRange:
                                if (empty)
                                {
                                    value.Set<valueTextRange>({ 0, 0 });
                                }
                                else
                                {
                                    value.Set<valueTextRange>({ 10, 20 });
                                }
                                break;

                            default:
                                VERIFY_FAIL(L"add case");
                                break;
                        }
                    }

                    static void SetIPVValue(ValueType valueType, ::CValue& value)
                    {
                        switch (valueType)
                        {
                            case valueBool:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateBoolean(true, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueSigned:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateInt32(-32, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueUnsigned:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateUInt32(33, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueInt64:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateInt64(-64, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueUInt64:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateUInt64(65, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueFloat:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateSingle(1.1111f, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueDouble:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateDouble(2.2222, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueString:
                                {
                                    wrl_wrappers::HString str;
                                    str.Set(L"wrappedstring");

                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateString(str, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueDateTime:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateDateTime({ 999 }, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            case valueTimeSpan:
                                {
                                    ctl::ComPtr<IInspectable> spValue;
                                    Private::ValueBoxer::CreateTimeSpan({ 1313 }, spValue.ReleaseAndGetAddressOf());
                                    value.Set<valueIInspectable>(spValue.Detach());
                                }
                                break;

                            default:
                                VERIFY_FAIL(L"add case");
                                break;
                        }
                    }


                    static void ValidateCValueReset(const ::CValue& value)
                    {
                        VERIFY_ARE_EQUAL(value.GetType(), valueAny);
                        VERIFY_ARE_EQUAL(value.OwnsValue(), false);
                        VERIFY_ARE_EQUAL((uint32_t)value.GetCustomData(), 0);
                    }

                    static void VerifyDeallocated(void* ptr, size_t size)
                    {
#ifdef DBG
                        bool same = true;

                        for (size_t s = 0; s < size; ++s)
                        {
                            same &= reinterpret_cast<uint8_t*>(ptr)[s] == 0xCC;
                        }

                        if (!same)
                        {
                            VERIFY_FAIL(L"Memory not deallocated");
                        }
#endif
                    }

                    template <typename T>
                    static void VerifyRefCount(T* obj, size_t expectedRefCount)
                    {
                        obj->AddRef();
                        auto actualRefCount = obj->Release();

                        if (actualRefCount != expectedRefCount)
                        {
                            VERIFY_ARE_EQUAL(actualRefCount, expectedRefCount);
                        }
                    }

                    template<>
                    static void VerifyRefCount<CDependencyObject>(CDependencyObject* obj, size_t expectedRefCount)
                    {
                        auto actualRefCount = obj->GetRefCount();

                        if (actualRefCount != expectedRefCount)
                        {
                            VERIFY_ARE_EQUAL(actualRefCount, expectedRefCount);
                        }
                    }

                    template <ValueType valueType>
                    static void ValidateCopy(const ::CValue& dest, const ::CValue& source, tag_selector<Kind::Empty>)
                    {}

                    template <ValueType valueType>
                    static void ValidateCopy(const ::CValue& dest, const ::CValue& source, tag_selector<Kind::Value>)
                    {
                        VERIFY_ARE_EQUAL(source.As<valueType>(), dest.As<valueType>());
                    }

                    template <ValueType valueType>
                    static void ValidateCopy(const ::CValue& dest, const ::CValue& source, tag_selector<Kind::String>)
                    {
                        VERIFY_IS_TRUE(source.As<valueString>().Equals(L"ABC"));
                        VERIFY_IS_TRUE(dest.As<valueString>().Equals(L"ABC"));
                    }

                    template <ValueType valueType>
                    static void ValidateCopy(const ::CValue& dest, const ::CValue& source, tag_selector<Kind::Ref>)
                    {
                        VERIFY_ARE_NOT_EQUAL(source.As<valueType>(), dest.As<valueType>());
                        VERIFY_ARE_EQUAL(*source.As<valueType>(), *dest.As<valueType>());
                    }

                    template <ValueType valueType>
                    static void ValidateCopy(const ::CValue& dest, const ::CValue& source, tag_selector<Kind::RefCount>)
                    {
                        VERIFY_ARE_EQUAL(source.As<valueType>(), dest.As<valueType>());
                    }

                    template <ValueType valueType>
                    static void ValidateCopy(const ::CValue& dest, const ::CValue& source, tag_selector<Kind::Array>)
                    {
                        VERIFY_ARE_EQUAL(source.GetArrayElementCount(), dest.GetArrayElementCount());
                        VERIFY_ARE_NOT_EQUAL(source.As<valueType>(), dest.As<valueType>());
                        VERIFY_ARE_EQUAL(memcmp(source.As<valueType>(), dest.As<valueType>(), 3 * sizeof(CValueDetails::ValueTypeInfo<valueType>::Type)), 0);
                    }

                    template <ValueType valueType>
                    static void ValidateMove(const ::CValue& dest, typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::Empty>)
                    {}

                    template <ValueType valueType>
                    static void ValidateMove(const ::CValue& dest, typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::Value>)
                    {
                        VERIFY_ARE_EQUAL(dest.As<valueType>(), value);
                    }

                    template <ValueType valueType>
                    static void ValidateMove(const ::CValue& dest, typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::String>)
                    {
                        VERIFY_IS_TRUE(dest.As<valueString>().Equals(value));
                    }

                    template <ValueType valueType>
                    static void ValidateMove(const ::CValue& dest, typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::Ref>)
                    {
                        VERIFY_ARE_EQUAL(dest.As<valueType>(), value);
                    }

                    template <ValueType valueType>
                    static void ValidateMove(const ::CValue& dest, typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::RefCount>)
                    {
                        VERIFY_ARE_EQUAL(dest.As<valueType>(), value);
                    }

                    template <ValueType valueType>
                    static void ValidateMove(const ::CValue& dest, typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::Array>)
                    {
                        VERIFY_ARE_EQUAL(dest.As<valueType>(), value);
                    }

                    template <ValueType valueType>
                    static void ValidateDealloc(typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::Empty>)
                    {}

                    template <ValueType valueType>
                    static void ValidateDealloc(typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::Value>)
                    {}

                    template <ValueType valueType>
                    static void ValidateDealloc(typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::String>)
                    {}

                    template <ValueType valueType>
                    static void ValidateDealloc(typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::Ref>)
                    {
                        VerifyDeallocated(value, sizeof(*value));
                    }

                    template <ValueType valueType>
                    static void ValidateDealloc(typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::RefCount>)
                    {
                        VerifyDeallocated(value, sizeof(*value));
                    }

                    template <ValueType valueType>
                    static void ValidateDealloc(typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, tag_selector<Kind::Array>)
                    {
                        VerifyDeallocated(value, 3 * sizeof(*value));
                    }

                    // Tests

                    template <ValueType valueType>
                    static void ValidateCopyCtorScenario()
                    {
                        typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value;

                        {
                            ::CValue source;

                            SetValue(valueType, source, false);
                            source.GetCustomData().SetIsSetLocally(true);

                            value = source.As<valueType>();

                            ::CValue dest(source);

                            VERIFY_ARE_EQUAL(source.GetType(), dest.GetType());
                            VERIFY_ARE_EQUAL(source.OwnsValue(), dest.OwnsValue());
                            VERIFY_ARE_EQUAL((uint32_t)source.GetCustomData(), (uint32_t)dest.GetCustomData());

                            ValidateCopy<valueType>(dest, source, tag_selector<selector<valueType>::kind>());
                        }

                        ValidateDealloc<valueType>(value, tag_selector<selector<valueType>::kind>());
                    }

                    template <ValueType valueType>
                    static void ValidateCopyCtorScenarioRefCounted()
                    {
                        typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value;

                        {
                            ::CValue source;

                            SetValue(valueType, source, false);
                            source.GetCustomData().SetIsSetLocally(true);

                            value = source.As<valueType>();
                            VerifyRefCount(value, 1);

                            ::CValue dest(source);

                            VerifyRefCount(value, 2); // copied
                            VERIFY_ARE_EQUAL(source.GetType(), dest.GetType());
                            VERIFY_ARE_EQUAL(source.OwnsValue(), dest.OwnsValue());
                            VERIFY_ARE_EQUAL((uint32_t)source.GetCustomData(), (uint32_t)dest.GetCustomData());

                            ValidateCopy<valueType>(dest, source, tag_selector<selector<valueType>::kind>());
                        }

                        ValidateDealloc<valueType>(value, tag_selector<selector<valueType>::kind>());
                    }

                    void CValueUnitTests::ValidateCopyCtor()
                    {
                        ValidateCopyCtorScenario<valueAny>();
                        ValidateCopyCtorScenario<valueNull>();
                        ValidateCopyCtorScenario<valueBool>();
                        ValidateCopyCtorScenario<valueDouble>();
                        ValidateCopyCtorScenario<valueString>();
                        ValidateCopyCtorScenario<valueDateTime>();
                        ValidateCopyCtorScenario<valuePoint>();
                        ValidateCopyCtorScenario<valueTextRange>();
                        ValidateCopyCtorScenario<valueFloatArray>();
                        ValidateCopyCtorScenarioRefCounted<valueObject>();
                        ValidateCopyCtorScenarioRefCounted<valueVO>();
                    }

                    template <ValueType valueType>
                    static void ValidateMoveCtorScenario()
                    {
                        typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value;

                        {
                            ::CValue source;

                            SetValue(valueType, source, false);
                            source.GetCustomData().SetIsSetLocally(true);

                            bool sourceOwns = source.OwnsValue();
                            value = source.As<valueType>();
                            uint32_t flags = (uint32_t)source.GetCustomData();
                            uint32_t count = source.GetArrayElementCount();

                            ::CValue dest(std::move(source));

                            ValidateCValueReset(source);
                            VERIFY_ARE_EQUAL(dest.GetType(), valueType);
                            VERIFY_ARE_EQUAL(dest.OwnsValue(), sourceOwns);
                            VERIFY_ARE_EQUAL(dest.GetArrayElementCount(), count);
                            VERIFY_ARE_EQUAL((uint32_t)dest.GetCustomData(), flags);

                            ValidateMove<valueType>(dest, value, tag_selector<selector<valueType>::kind>());
                        }

                        ValidateDealloc<valueType>(value, tag_selector<selector<valueType>::kind>());
                    }

                    template <ValueType valueType>
                    static void ValidateMoveCtorScenarioRefCounted()
                    {
                        typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value;

                        {
                            ::CValue source;

                            SetValue(valueType, source, false);
                            source.GetCustomData().SetIsSetLocally(true);

                            value = source.As<valueType>();
                            uint32_t flags = (uint32_t)source.GetCustomData();

                            VerifyRefCount(value, 1); // didn't add...

                            ::CValue dest(std::move(source));

                            ValidateCValueReset(source);
                            VerifyRefCount(value, 1); // didn't add

                            VERIFY_ARE_EQUAL(dest.GetType(), valueType);
                            VERIFY_ARE_EQUAL(dest.OwnsValue(), true);
                            VERIFY_ARE_EQUAL(dest.GetArrayElementCount(), 0);
                            VERIFY_ARE_EQUAL((uint32_t)dest.GetCustomData(), flags);

                            ValidateMove<valueType>(dest, value, tag_selector<selector<valueType>::kind>());
                        }

                        ValidateDealloc<valueType>(value, tag_selector<selector<valueType>::kind>());
                    }

                    void CValueUnitTests::ValidateMoveCtor()
                    {
                        ValidateMoveCtorScenario<valueAny>();
                        ValidateMoveCtorScenario<valueNull>();
                        ValidateMoveCtorScenario<valueBool>();
                        ValidateMoveCtorScenario<valueDouble>();
                        ValidateMoveCtorScenario<valueString>();
                        ValidateMoveCtorScenario<valueDateTime>();
                        ValidateMoveCtorScenario<valuePoint>();
                        ValidateMoveCtorScenario<valueTextRange>();
                        ValidateMoveCtorScenario<valueFloatArray>();
                        ValidateMoveCtorScenarioRefCounted<valueObject>();
                        ValidateMoveCtorScenarioRefCounted<valueVO>();
                    }

                    template <ValueType valueType>
                    static void ValidateIsNullArray(typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType value, uint32_t count)
                    {
                        ::CValue source;

                        source.SetArray<valueType>(count, value);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.SetArray<valueType>(0, nullptr);
                        VERIFY_IS_TRUE(source.IsNull());
                    }

                    template <ValueType valueType>
                    static void ValidateIsNullReferenceWrap()
                    {
                        ::CValue source;

                        source.Wrap<valueType>(reinterpret_cast<typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType>(1));
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueType>(nullptr);
                        VERIFY_IS_TRUE(source.IsNull());
                    }

                    template <ValueType valueType>
                    static void ValidateIsNullReferenceSet()
                    {
                        ::CValue source;

                        source.Set<valueType>(reinterpret_cast<typename ::CValueDetails::ValueTypeInfo<valueType>::Store::MappedType>(1));
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueType>(nullptr);
                        VERIFY_IS_TRUE(source.IsNull());
                    }

                    void CValueUnitTests::ValidateIsNull()
                    {
                        ::CValue source;

                        VERIFY_IS_FALSE(source.IsNull());

                        source.SetNull();
                        VERIFY_IS_TRUE(source.IsNull());

                        source.SetEnum(xaml::HorizontalAlignment::HorizontalAlignment_Center, KnownTypeIndex::HorizontalAlignment);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueBool>(false);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueSigned>(0);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueUnsigned>(0);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueInt64>(0);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueUInt64>(0);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueFloat>(0);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueDouble>(0);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueColor>(0);
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueDateTime>({ 0 });
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueTimeSpan>({ 0 });
                        VERIFY_IS_FALSE(source.IsNull());

                        source.Set<valueTextRange>({ 0, 0 });
                        VERIFY_IS_FALSE(source.IsNull());

                        ValidateIsNullArray<valueSignedArray>(new int32_t[2]{ 1, 2 }, 2);
                        ValidateIsNullArray<valueFloatArray>(new float[2]{ 1.3f, 1.2f }, 2);
                        ValidateIsNullArray<valueDoubleArray>(new double[2]{ 1.3, 1.2 }, 2);
                        ValidateIsNullArray<valuePointArray>(new XPOINTF[2]{ {1.3f, 1.2f},{ 1.3f, 1.2f } }, 2);

                        ValidateIsNullReferenceWrap<valuePoint>();
                        ValidateIsNullReferenceWrap<valueSize>();
                        ValidateIsNullReferenceWrap<valueRect>();
                        ValidateIsNullReferenceWrap<valueThickness>();
                        ValidateIsNullReferenceWrap<valueGridLength>();
                        ValidateIsNullReferenceWrap<valueCornerRadius>();
                        ValidateIsNullReferenceWrap<valueObject>();
                        ValidateIsNullReferenceWrap<valueVO>();
                        ValidateIsNullReferenceWrap<valueIUnknown>();
                        ValidateIsNullReferenceWrap<valueIInspectable>();
                        ValidateIsNullReferenceWrap<valueThemeResource>();

                        ValidateIsNullReferenceSet<valueInternalHandler>();
                        ValidateIsNullReferenceSet<valuePointer>();

                        {
                            ::CValue source2;

                            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(str, L"string");

                            source2.Set<valueString>(str);
                            VERIFY_IS_FALSE(source2.IsNull());

                            source2.Set<valueString>(xstring_ptr::NullString());
                            VERIFY_IS_TRUE(source2.IsNull());
                        }

                        {
                            ::CValue source2;

                            source2.Set<valueTypeHandle>(KnownTypeIndex::AlignmentX);
                            VERIFY_IS_FALSE(source2.IsNull());

                            source2.Set<valueTypeHandle>(KnownTypeIndex::UnknownType);
                            VERIFY_IS_TRUE(source2.IsNull());
                        }
                    }

                    void CValueUnitTests::ValidateIsUnset()
                    {
                        ::CValue source;

                        VERIFY_IS_TRUE(source.IsUnset());

                        source.SetNull();
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.SetEnum(xaml::HorizontalAlignment::HorizontalAlignment_Center, KnownTypeIndex::HorizontalAlignment);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueBool>(false);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueSigned>(0);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueUnsigned>(0);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueInt64>(0);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueUInt64>(0);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueFloat>(0);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueDouble>(0);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueColor>(0);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueDateTime>({ 0 });
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueTimeSpan>({ 0 });
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.SetArray<valueSignedArray>(0, nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.SetArray<valueFloatArray>(0, nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.SetArray<valueDoubleArray>(0, nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.SetArray<valuePointArray>(0, nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valuePoint>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueSize>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueRect>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueThickness>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueGridLength>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueCornerRadius>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueInternalHandler>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valuePointer>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueObject>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueVO>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueIUnknown>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueIInspectable>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueThemeResource>(nullptr);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueTextRange>({ 0, 0 });
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueTypeHandle>(KnownTypeIndex::UnknownType);
                        VERIFY_IS_FALSE(source.IsUnset());

                        source.Set<valueString>(xstring_ptr::NullString());
                        VERIFY_IS_FALSE(source.IsUnset());
                    }

                    void CValueUnitTests::ValidateIsFloatingPoint()
                    {
                        ::CValue source;

                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.SetNull();
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.SetEnum(xaml::HorizontalAlignment::HorizontalAlignment_Center, KnownTypeIndex::HorizontalAlignment);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueBool>(false);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueSigned>(0);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueUnsigned>(0);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueInt64>(0);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueUInt64>(0);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueFloat>(0);
                        VERIFY_IS_TRUE(source.IsFloatingPoint());

                        source.Set<valueDouble>(0);
                        VERIFY_IS_TRUE(source.IsFloatingPoint());

                        source.Set<valueColor>(0);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueDateTime>({ 0 });
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueTimeSpan>({ 0 });
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.SetArray<valueSignedArray>(0, nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.SetArray<valueFloatArray>(0, nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.SetArray<valueDoubleArray>(0, nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.SetArray<valuePointArray>(0, nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valuePoint>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueSize>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueRect>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueThickness>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueGridLength>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueCornerRadius>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueInternalHandler>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valuePointer>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueObject>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueVO>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueIUnknown>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueIInspectable>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueThemeResource>(nullptr);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueTypeHandle>(KnownTypeIndex::UnknownType);
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueString>(xstring_ptr::NullString());
                        VERIFY_IS_FALSE(source.IsFloatingPoint());

                        source.Set<valueTextRange>({ 0, 0 });
                        VERIFY_IS_FALSE(source.IsFloatingPoint());
                    }

                    void CValueUnitTests::ValidateIsArray()
                    {
                        ::CValue source;

                        VERIFY_IS_FALSE(source.IsArray());

                        source.SetNull();
                        VERIFY_IS_FALSE(source.IsArray());

                        source.SetEnum(xaml::HorizontalAlignment::HorizontalAlignment_Center, KnownTypeIndex::HorizontalAlignment);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueBool>(false);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueSigned>(0);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueUnsigned>(0);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueInt64>(0);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueUInt64>(0);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueFloat>(0);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueDouble>(0);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueColor>(0);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueDateTime>({ 0 });
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueTimeSpan>({ 0 });
                        VERIFY_IS_FALSE(source.IsArray());

                        source.SetArray<valueSignedArray>(0, nullptr);
                        VERIFY_IS_TRUE(source.IsArray());

                        source.SetArray<valueFloatArray>(0, nullptr);
                        VERIFY_IS_TRUE(source.IsArray());

                        source.SetArray<valueDoubleArray>(0, nullptr);
                        VERIFY_IS_TRUE(source.IsArray());

                        source.SetArray<valuePointArray>(0, nullptr);
                        VERIFY_IS_TRUE(source.IsArray());

                        source.Set<valuePoint>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueSize>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueRect>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueThickness>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueGridLength>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueCornerRadius>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueInternalHandler>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valuePointer>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueObject>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueVO>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueIUnknown>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueIInspectable>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueThemeResource>(nullptr);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueTypeHandle>(KnownTypeIndex::UnknownType);
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueString>(xstring_ptr::NullString());
                        VERIFY_IS_FALSE(source.IsArray());

                        source.Set<valueTextRange>({ 0, 0 });
                        VERIFY_IS_FALSE(source.IsArray());
                    }

#define VALUETYPE_ENTRY(x)  { x, L#x }
                    static PCWSTR GetValueTypeName(ValueType type)
                    {
                        struct {
                            ValueType type;
                            PCWSTR name;
                        } const c_map[] = {
                                VALUETYPE_ENTRY(valueAny),
                                VALUETYPE_ENTRY(valueNull),
                                VALUETYPE_ENTRY(valueBool),    // Boolean value.
                                VALUETYPE_ENTRY(valueEnum),    // 32 bit integer enumeration.

                                VALUETYPE_ENTRY(valueSigned),    // 32 bit signed integer value.
                                VALUETYPE_ENTRY(valueUnsigned),    // 32 bit unsigned integer value.
                                VALUETYPE_ENTRY(valueInt64),    // 64 bit signed integer value.
                                VALUETYPE_ENTRY(valueUInt64),    // 64 bit unsigned integer value.

                                VALUETYPE_ENTRY(valueFloat),    // 32 bit float value.
                                VALUETYPE_ENTRY(valueDouble),    // 64 bit double-precision float value

                                VALUETYPE_ENTRY(valueSignedArray),   // Length specified float array.
                                VALUETYPE_ENTRY(valueFloatArray),   // Length specified float array.
                                VALUETYPE_ENTRY(valueDoubleArray),   // Length specified double array.
                                VALUETYPE_ENTRY(valuePointArray),   // Length specified array of Point (pair of float)

                                VALUETYPE_ENTRY(valueString),   // Length specified UNICODE string.

                                VALUETYPE_ENTRY(valueColor),   // A8R8G8B8 value.
                                VALUETYPE_ENTRY(valuePoint),   // Pair of float values.
                                VALUETYPE_ENTRY(valueSize),   // Size (can be handled like Point for the most part)
                                VALUETYPE_ENTRY(valueRect),   // Pair of point values.
                                VALUETYPE_ENTRY(valueThickness),   // left, top, right, bottom
                                VALUETYPE_ENTRY(valueGridLength),   // GridLength for specifying grid row/column dimensions
                                VALUETYPE_ENTRY(valueCornerRadius),   // Used to describe the radius of rectangle's corners
                                VALUETYPE_ENTRY(valueDateTime),   // DateTime
                                VALUETYPE_ENTRY(valueTimeSpan),   // TimeSpan

                                VALUETYPE_ENTRY(valueObject),   // Reference counted CDependencyObject.
                                VALUETYPE_ENTRY(valueInternalHandler),   // Static method pointer to internal code
                                VALUETYPE_ENTRY(valueIUnknown),   // IUnknown object, that is not CDependencyObject
                                VALUETYPE_ENTRY(valueIInspectable),   // IInspectable object, that is not CDependencyObject

                                VALUETYPE_ENTRY(valueTypeHandle),   // KnownTypeIndex
                                VALUETYPE_ENTRY(valueThemeResource),   // Pointer to theme resource, managed lifetime.

                                VALUETYPE_ENTRY(valuePointer),   // Raw pointer, no type information, no lifetime management, no reference counting - use at your own risk!

                                VALUETYPE_ENTRY(valueVO),   // Value Objects

                                VALUETYPE_ENTRY(valueTextRange),   // TextRange struct with start index and length
                                VALUETYPE_ENTRY(valueEnum8),
                                VALUETYPE_ENTRY(valueTypeSentinel)       // Always last...
                        };
                        return std::find_if(std::begin(c_map), std::end(c_map), [type](const auto& item) { return item.type == type; })->name;
                    }

                    static void ValidateCompareScenario(bool empty1, bool empty2)
                    {
                        bool good;

                        for (ValueType i = static_cast<ValueType>(0); i < valueTypeSentinel; i = static_cast<ValueType>(static_cast<uint32_t>(i) + 1))
                        {
                            ::CValue v0;
                            SetValue(i, v0, empty1);

                            for (ValueType j = static_cast<ValueType>(0); j < valueTypeSentinel; j = static_cast<ValueType>(static_cast<uint32_t>(j) + 1))
                            {
                                ::CValue v1;
                                SetValue(j, v1, empty2);

                                if ((i == valueAny && j == valueAny) ||
                                    (i == valueSignedArray && j == valueSignedArray) ||
                                    (i == valueFloatArray && j == valueFloatArray) ||
                                    (i == valueDoubleArray && j == valueDoubleArray) ||
                                    (i == valuePointArray && j == valuePointArray) ||
                                    (i == valueObject && j == valueObject) ||
                                    (i == valueVO && j == valueVO) ||
                                    (i == valueIInspectable && j == valueIInspectable) ||
                                    (i == valueIUnknown && j == valueIUnknown))
                                {
                                    good = (v0 != v1);
                                }
                                else if (i == j)
                                {
                                    good = (v0 == v1);
                                }
                                else
                                {
                                    if ((i == valueEnum && j == valueEnum8) ||
                                        (j == valueEnum && i == valueEnum8))
                                    {
                                        good = v0 == v1;
                                    }
                                    else
                                    {
                                        good = (v0 != v1);
                                    }
                                }

                                if (!good)
                                {
                                    LOG_OUTPUT(L"Comparison failed for %s and %s", GetValueTypeName(i), GetValueTypeName(j));
                                    VERIFY_FAIL(L"FAILED!");
                                }
                            }
                        }
                    }

                    void CValueUnitTests::ValidateCompare()
                    {
                        ValidateCompareScenario(false, false);
                    }

                    static void ValidateCompareWrappedIPVScenario(ValueType valueType)
                    {
                        IInspectable* rawV0 = nullptr;
                        IInspectable* rawV1 = nullptr;

                        {
                            ::CValue v0;
                            ::CValue v1;

                            SetIPVValue(valueType, v0);
                            SetIPVValue(valueType, v1);

                            rawV0 = v0.As<valueIInspectable>();
                            rawV1 = v1.As<valueIInspectable>();

                            VerifyRefCount(rawV0, 1);
                            VerifyRefCount(rawV1, 1);

                            VERIFY_IS_TRUE(v0 == v1);
                            VERIFY_IS_FALSE(CValueUtil::EqualsWithoutUnboxing(v0, v1));
                        }
                    }

                    void CValueUnitTests::ValidateCompareWrappedIPV()
                    {
                        ValidateCompareWrappedIPVScenario(valueBool);
                        ValidateCompareWrappedIPVScenario(valueSigned);
                        ValidateCompareWrappedIPVScenario(valueUnsigned);
                        ValidateCompareWrappedIPVScenario(valueInt64);
                        ValidateCompareWrappedIPVScenario(valueUInt64);
                        ValidateCompareWrappedIPVScenario(valueDouble);
                        ValidateCompareWrappedIPVScenario(valueFloat);
                        ValidateCompareWrappedIPVScenario(valueString);
                        ValidateCompareWrappedIPVScenario(valueTimeSpan);
                        ValidateCompareWrappedIPVScenario(valueDateTime);
                    }

                    void CValueUnitTests::ValidateAccessorsForValues()
                    {
                        ::CValue value;
                        const double e = 2.718;
                        const bool b = true;

                        value.Set<valueDouble>(e);

                        VERIFY_ARE_EQUAL(value.GetType(), valueDouble);

                        // As
                        auto t0 = value.As<valueDouble>();
                        VERIFY_ARE_EQUAL(t0, e);

                        // Get
                        t0 = 0;
                        VERIFY_SUCCEEDED(value.Get<valueDouble>(t0));
                        VERIFY_ARE_EQUAL(t0, e);

                        // As something else
                        auto t1 = value.As<valuePointer>();
                        VERIFY_IS_NULL(t1);

                        // Get something else
                        VERIFY_FAILED(value.Get<valuePointer>(t1));

                        value.Set<valueBool>(b);

                        VERIFY_ARE_EQUAL(value.GetType(), valueBool);

                        // As
                        auto t2 = value.As<valueBool>();
                        VERIFY_ARE_EQUAL(t2, b);

                        // Get
                        t2 = false;
                        VERIFY_SUCCEEDED(value.Get<valueBool>(t2));
                        VERIFY_ARE_EQUAL(t2, b);

                        // As something else
                        t1 = value.As<valuePointer>();
                        VERIFY_IS_NULL(t1);

                        // Get something else
                        VERIFY_FAILED(value.Get<valuePointer>(t1));
                    }

                    void CValueUnitTests::ValidateAccessorsForReferences()
                    {
                        ::CValue value;
                        XPOINTF* obj1 = new XPOINTF();
                        XPOINTF* obj2 = new XPOINTF();

                        // Take ownership of obj1
                        value.Set<valuePoint>(obj1);
                        VERIFY_ARE_EQUAL(obj1->x, 0.0f);

                        VERIFY_ARE_EQUAL(value.GetType(), valuePoint);

                        // As
                        XPOINTF* t0 = value.As<valuePoint>();
                        VERIFY_ARE_EQUAL(t0, obj1);
                        VERIFY_ARE_EQUAL(obj1->x, 0.0f);

                        // Get
                        t0 = nullptr;
                        VERIFY_SUCCEEDED(value.Get<valuePoint>(t0));
                        VERIFY_ARE_EQUAL(t0, obj1);
                        VERIFY_ARE_EQUAL(obj1->x, 0.0f);

                        // As something else
                        auto t1 = value.As<valuePointer>();
                        VERIFY_IS_NULL(t1);

                        // Get something else
                        VERIFY_FAILED(value.Get<valuePointer>(t1));

                        // Wrap obj2, release ref from obj1
                        value.Wrap<valuePoint>(obj2);
                        VerifyDeallocated(obj1, sizeof(XPOINTF));
                        VERIFY_ARE_EQUAL(obj2->x, 0.0f);

                        // Not owned, no destuction
                        value.ReleaseAndReset();
                        VERIFY_ARE_EQUAL(obj2->x, 0.0f);

                        value.Set<valuePoint>(obj2);
                        value.ReleaseAndReset();
                        VerifyDeallocated(obj2, sizeof(XPOINTF));
                    }

                    void CValueUnitTests::ValidateAccessorsForArrays()
                    {
                        ::CValue value;
                        XPOINTF *obj1 = new XPOINTF[2];
                        XPOINTF *obj2 = new XPOINTF[3];

                        obj1[0] = {};
                        obj1[1] = {};
                        obj2[0] = {};
                        obj2[1] = {};
                        obj2[2] = {};

                        // Take ownership of obj1
                        value.SetArray<valuePointArray>(2, obj1);
                        VERIFY_ARE_EQUAL(obj1[0].x, 0.0f);

                        VERIFY_ARE_EQUAL(value.GetType(), valuePointArray);
                        VERIFY_ARE_EQUAL(value.GetArrayElementCount(), 2);

                        // As
                        XPOINTF* t0 = value.As<valuePointArray>();
                        VERIFY_ARE_EQUAL(t0, obj1);
                        VERIFY_ARE_EQUAL(obj1[0].x, 0.0f);

                        // Get
                        uint32_t size = 0;
                        t0 = nullptr;
                        VERIFY_SUCCEEDED(value.GetArray<valuePointArray>(size, t0));
                        VERIFY_ARE_EQUAL(t0, obj1);
                        VERIFY_ARE_EQUAL(obj1[0].x, 0.0f);
                        VERIFY_ARE_EQUAL(size, 2);

                        // As something else
                        auto t1 = value.As<valuePointer>();
                        VERIFY_IS_NULL(t1);

                        // Get something else
                        VERIFY_FAILED(value.Get<valuePointer>(t1));

                        // Wrap obj2, release ref from obj1
                        value.WrapArray<valuePointArray>(3, obj2);
                        VerifyDeallocated(obj1, 2 * sizeof(XPOINTF));
                        VERIFY_ARE_EQUAL(value.GetArrayElementCount(), 3);
                        VERIFY_ARE_EQUAL(obj2[0].x, 0.0f);

                        // Not owned, no destuction
                        value.ReleaseAndReset();
                        VERIFY_ARE_EQUAL(obj2[0].x, 0.0f);

                        value.SetArray<valuePointArray>(3, obj2);
                        value.ReleaseAndReset();
                        VerifyDeallocated(obj2, 3 * sizeof(XPOINTF));
                    }

                    void CValueUnitTests::ValidateAccessorsForRefCounted()
                    {
                        ::CValue value;
                        CDependencyObject* obj1 = new CDependencyObject();
                        CDependencyObject* obj2 = new CDependencyObject();

                        VerifyRefCount(obj1, 1);
                        VerifyRefCount(obj2, 1);

                        // Take ownership of obj1 and addref
                        value.SetAddRef<valueObject>(obj1);
                        VerifyRefCount(obj1, 2);

                        VERIFY_ARE_EQUAL(value.GetType(), valueObject);

                        // As object
                        CDependencyObject* t0 = value.As<valueObject>();
                        VERIFY_ARE_EQUAL(t0, obj1);
                        VerifyRefCount(obj1, 2);

                        // Get object
                        t0 = nullptr;
                        VERIFY_SUCCEEDED(value.Get<valueObject>(t0));
                        VERIFY_ARE_EQUAL(t0, obj1);
                        VerifyRefCount(obj1, 2);

                        // As something else
                        auto t1 = value.As<valuePointer>();
                        VERIFY_IS_NULL(t1);

                        // Get something else
                        VERIFY_FAILED(value.Get<valuePointer>(t1));

                        // Wrap obj2, release ref from obj1
                        value.Wrap<valueObject>(obj2);
                        VerifyRefCount(obj1, 1);
                        VerifyRefCount(obj2, 1);

                        // Set to obj1, since obj2 was not add refed, leave it as is
                        value.Set<valueObject>(obj1);
                        VerifyRefCount(obj1, 1);
                        VerifyRefCount(obj2, 1);

                        // Set to obj2, obj1 was not add refed and it was owned by CValue - deallocate it
                        value.Set<valueObject>(obj2);
                        VerifyDeallocated(obj1, sizeof(CDependencyObject));
                        VerifyRefCount(obj2, 1);

                        value.ReleaseAndReset();
                        VerifyDeallocated(obj2, sizeof(CDependencyObject));
                    }

                    template <ValueType valueType>
                    static void ValidateWrapValueScenario()
                    {
                        ::CValue source;

                        SetValue(valueType, source, false);
                        source.GetCustomData().SetIsSetLocally(true);

                        ::CValue dest;

                        dest.WrapValue(source);

                        VERIFY_ARE_EQUAL(source.GetType(), dest.GetType());
                        VERIFY_ARE_EQUAL(dest.OwnsValue(), false);
                        VERIFY_ARE_EQUAL(source.As<valueType>(), dest.As<valueType>());
                        VERIFY_ARE_EQUAL((uint32_t)source.GetCustomData(), (uint32_t)dest.GetCustomData());
                    }

                    void CValueUnitTests::ValidateWrapValue()
                    {
                        ValidateWrapValueScenario<valueAny>();
                        ValidateWrapValueScenario<valueNull>();
                        ValidateWrapValueScenario<valueBool>();
                        ValidateWrapValueScenario<valueString>();
                        ValidateWrapValueScenario<valueSigned>();
                        ValidateWrapValueScenario<valueInt64>();
                        ValidateWrapValueScenario<valueDouble>();
                        ValidateWrapValueScenario<valueFloatArray>();
                        ValidateWrapValueScenario<valuePoint>();
                        ValidateWrapValueScenario<valueObject>();
                        ValidateWrapValueScenario<valueVO>();
                        ValidateWrapValueScenario<valuePointer>();
                        ValidateWrapValueScenario<valueTextRange>();
                    }

                    template <ValueType valueType>
                    static void ValidateCopyValueScenario()
                    {
                        ::CValue source;

                        SetValue(valueType, source, false);
                        source.GetCustomData().SetIsSetLocally(true);

                        ::CValue dest;

                        dest.CopyValue(source);

                        VERIFY_ARE_EQUAL(source.GetType(), dest.GetType());
                        VERIFY_ARE_EQUAL(dest.OwnsValue(), true);
                        VERIFY_ARE_EQUAL((uint32_t)source.GetCustomData(), (uint32_t)dest.GetCustomData());

                        ValidateCopy<valueType>(dest, source, tag_selector<selector<valueType>::kind>());
                    }

                    void CValueUnitTests::ValidateCopyValue()
                    {
                        ValidateCopyValueScenario<valueAny>();
                        ValidateCopyValueScenario<valueNull>();
                        ValidateCopyValueScenario<valueBool>();
                        ValidateCopyValueScenario<valueString>();
                        ValidateCopyValueScenario<valueSigned>();
                        ValidateCopyValueScenario<valueInt64>();
                        ValidateCopyValueScenario<valueDouble>();
                        ValidateCopyValueScenario<valueFloatArray>();
                        ValidateCopyValueScenario<valuePoint>();
                        ValidateCopyValueScenario<valueObject>();
                        ValidateCopyValueScenario<valueVO>();
                        ValidateCopyValueScenario<valuePointer>();
                        ValidateCopyValueScenario<valueTextRange>();
                    }

                    void CValueUnitTests::CopyFromDouble()
                    {
                        ::CValue value;
                        value.SetDouble(42.5);

                        ::CValue newValue;
                        VERIFY_SUCCEEDED(newValue.CopyConverted(value));
                        VERIFY_ARE_EQUAL(valueDouble, value.GetType());
                        VERIFY_ARE_EQUAL(42.5, value.AsDouble());
                    }

                    void CValueUnitTests::FloatAsDouble()
                    {
                        ::CValue value;
                        value.SetFloat(42.5f);
                        VERIFY_ARE_EQUAL(42.5, value.AsDouble());
                    }

                    void CValueUnitTests::GetSetColor()
                    {
                        ::CValue value;
                        unsigned int getColor = 1234;

                        VERIFY_FAILED(value.GetColor(getColor));
                        VERIFY_ARE_EQUAL(0, getColor);

                        value.SetColor(0xff00ff00);
                        VERIFY_SUCCEEDED(value.GetColor(getColor));
                        VERIFY_ARE_EQUAL(0xff00ff00, getColor);
                    }

                    void CValueUnitTests::UnknownTypeIsNull()
                    {
                        ::CValue value;
                        value.SetTypeHandle(KnownTypeIndex::UnknownType);
                        VERIFY_IS_TRUE(!!value.IsNull());
                    }

                    void CValueUnitTests::CanCorrectlyCompareDateAndTime()
                    {
                        ::CValue value1, value2;
                        wf::DateTime dt;
                        dt.UniversalTime = 123456789l;
                        value1.SetDateTime(dt);
                        value2.SetDateTime(dt);
                        VERIFY_IS_TRUE(value1 == value2);

                        wf::TimeSpan ts;
                        ts.Duration = 123456789l;
                        value1.SetTimeSpan(ts);
                        value2.SetTimeSpan(ts);
                        VERIFY_IS_TRUE(value1 == value2);
                    }

                }
            }
        }
    }
}
