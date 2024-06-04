// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CValue.h>
#include <CDependencyObject.h>
#include <CValueConvert.h>
#include <MetadataAPI.h>

using namespace CValueDetails;

// Generic dispatch for value types.
// When adding new types, copy and paste a line in the switch statement.
template <typename Handler, typename OutType, typename V>
OutType CValue::Dispatch(
    _Inout_ ValueType valueType,
    _Inout_ V&& arg)
{
    static constexpr unsigned baseCounter = __COUNTER__ + 1;

    switch (static_cast<unsigned>(valueType))
    {

#define DECLARE_INVOKE(N) case (N): \
        static_assert(N < static_cast<unsigned>(valueTypeSentinel), "Something went wrong, case value out of range."); \
        return Handler::template invoke<static_cast<ValueType>(N)>(*this, std::forward<V>(arg));

        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 0
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 1
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 2
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 3
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 4
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 5
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 6
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 7
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 8
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 9
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 10
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 11
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 12
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 13
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 14
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 15
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 16
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 17
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 18
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 19
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 20
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 21
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 22
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 23
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 24
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 25
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 26
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 27
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 28
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 29
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 30
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 31
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 32
        DECLARE_INVOKE(__COUNTER__ - baseCounter); // 33

#undef DECLARE_INVOKE

        default:
            XCP_FAULT_ON_FAILURE(false);
            return OutType();
    }

    static_assert(valueTypeSentinel == (__COUNTER__ - baseCounter), "Add DECLARE_INVOKE(__COUNTER__ - baseCounter) above.");
}

namespace CValueDetails
{
    struct tag_move {};
    struct tag_shallow {};
    struct tag_deep {};

    struct Handlers
    {
        // Handles set/copy/move requests.  Responsible for setting CValue internal flags, but not custom flags.
        template <typename CopyOp>
        struct Transfer
        {
            template <ValueType sourceValueType>
            static void TransferToStore(
                _Out_ CValue& target,
                _Inout_ CValue&& source,
                tag_move)
            {
                ValueTypeInfo<sourceValueType>::Store::Move(
                    target,
                    std::move(source));

                target.SetOwnsValue(source.OwnsValue());
            }

            template <ValueType sourceValueType>
            static void TransferToStore(
                _Out_ CValue& target,
                _In_ const CValue& source,
                tag_shallow)
            {
                ValueTypeInfo<sourceValueType>::Store::Set(
                    target,
                    ValueTypeInfo<sourceValueType>::Store::Get(source));

                target.SetOwnsValue(false);
            }

            template <>
            static void TransferToStore<valueString>(
                _Out_ CValue& target,
                _In_ const CValue& source,
                tag_shallow)
            {
                using Accessor = ValueTypeInfo<valueString>::Store::Accessor;

                Accessor::Set(
                    GetField<valueString>(target.m_value),
                    Accessor::Get(GetField<valueString>(source.m_value)));

                target.SetOwnsValue(false);
            }

            template <ValueType sourceValueType>
            static void TransferToStore(
                _Out_ CValue& target,
                _In_ const CValue& source,
                tag_deep)
            {
                ValueTypeInfo<sourceValueType>::Store::Copy(
                    target,
                    source);

                target.SetOwnsValue(true);
            }

            template <ValueType sourceValueType, typename V>
            static void invoke(
                _Out_ CValue& target,
                _Inout_ V&& source)
            {
                ASSERT(source.GetType() == sourceValueType);

                // Invoke appropriate method for setting value.
                TransferToStore<sourceValueType>(
                    target,
                    std::forward<V>(source),
                    CopyOp());

                target.SetType(sourceValueType);

                if (ValueTypeInfo<sourceValueType>::Store::isArray)
                {
                    target.SetArrayElementCount(
                        source.GetArrayElementCountInternal());
                }
            }
        };

        // Handles copy with CDependencyObject unboxing.  Responsible for setting CValue internal flags, but not custom flags.
        struct ConvertAndCopy
        {
            // General case implementation - no conversion, just copy value.
            template <ValueType sourceValueType>
            static _Check_return_ HRESULT Copy(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == sourceValueType);

                ValueTypeInfo<sourceValueType>::Store::Copy(
                    target,
                    source);

                target.SetType(sourceValueType);
                target.SetOwnsValue(true);

                if (ValueTypeInfo<sourceValueType>::Store::isArray)
                {
                    target.SetArrayElementCount(
                        source.GetArrayElementCountInternal());
                }

                return S_OK;
            }

            // Specialization for valueAny, as it does not own its value.  Internal flags should be zeroed, so this is a noop.
            template <>
            STATIC_SPEC _Check_return_ HRESULT Copy<valueAny>(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueAny);
                return S_OK;
            }

            // For valueObject try unboxing the value.  EnsureCDependencyObjectValueUnboxed should set internal flags.
            template <>
            STATIC_SPEC _Check_return_ HRESULT Copy<valueObject>(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueObject);

                IFC_RETURN(CValueConvert::EnsureCDependencyObjectValueUnboxed(
                    source,
                    target));

                return S_OK;
            }

            // For valueVO try unboxing the value.  EnsureValueObjectUnboxed should set internal flags.
            template <>
            STATIC_SPEC _Check_return_ HRESULT Copy<valueVO>(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueVO);

                IFC_RETURN(CValueConvert::EnsureValueObjectUnboxed(
                    source,
                    target));

                return S_OK;
            }

            template <ValueType sourceValueType>
            static _Check_return_ HRESULT invoke(
                _Out_ CValue& target,
                _In_ const CValue& source)
            {
                return Copy<sourceValueType>(
                    target,
                    source);
            }
        };

        // Handler for value comparison.
        struct Compare
        {
            // General case implementation.
            template <ValueType sourceValueType>
            static bool Equals(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == sourceValueType);

                if (target.GetType() == sourceValueType)
                {
                    // By default delegate to the store.
                    return ValueTypeInfo<sourceValueType>::Store::Compare(target, source);
                }
                else
                {
                    return false;
                }
            }

            template <>
            STATIC_SPEC bool Equals<valueEnum>(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueEnum);

                switch (target.GetType())
                {
                    case valueEnum:
                        // Can do direct comparison, so directly call into Store.
                        return ValueTypeInfo<valueEnum>::Store::Compare(target, source);

                    case valueEnum8:
                        // promote target
                        return target.As<valueEnum>() == source.As<valueEnum>();
                }

                return false;
            }

            template <>
            STATIC_SPEC bool Equals<valueEnum8>(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueEnum8);

                switch (target.GetType())
                {
                    case valueEnum8:
                        // Can do direct comparison, so directly call into Store.
                        return ValueTypeInfo<valueEnum8>::Store::Compare(target, source);

                    case valueEnum:
                        // promote source
                        return target.As<valueEnum>() == source.As<valueEnum>();
                }

                return false;
            }


            // These checks are added to prevent additional OnPropertyChanged notifications from
            // being fired for the case where we assign a binding to a property. The binding expression
            // unwraps into a valueDouble CValue while the default CValue type of the GetDefaultValue
            // codepath creates valueFloat CValues for Double properties.
            //
            // A follow up bug, MSFT: 810951, has been opened to fix GetDefaultValue. The fix was considered
            // too high-risk for this M1.3 bug.
            template <>
            STATIC_SPEC bool Equals<valueFloat>(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueFloat);

                switch (target.GetType())
                {
                    case valueFloat:
                        // Can do direct comparison, so directly call into Store.
                        return ValueTypeInfo<valueFloat>::Store::Compare(target, source);

                    case valueDouble:
                        return target.As<valueDouble>() == source.As<valueFloat>();
                }

                return false;
            }

            template <>
            STATIC_SPEC bool Equals<valueDouble>(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueDouble);

                switch (target.GetType())
                {
                    case valueFloat:
                        return target.As<valueFloat>() == source.As<valueDouble>();

                    case valueDouble:
                        // Can do direct comparison, so directly call into Store.
                        return ValueTypeInfo<valueDouble>::Store::Compare(target, source);
                }

                return false;
            }

            // For valueIInspectable try unboxing IPropertyValue and perform comparison on unboxed value.
            template <>
            STATIC_SPEC bool Equals<valueIInspectable>(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueIInspectable);

                if (target.GetType() == valueIInspectable)
                {
                    CValue targetUnboxed;
                    CValue sourceUnboxed;

                    VERIFYHR(CValueConvert::EnsurePropertyValueUnboxed(
                        target,
                        targetUnboxed));

                    VERIFYHR(CValueConvert::EnsurePropertyValueUnboxed(
                        source,
                        sourceUnboxed));

                    if ((targetUnboxed.GetType() == valueIInspectable) ^ (sourceUnboxed.GetType() == valueIInspectable))
                    {
                        // One is valueIInspectable the other is not - can't be equal.
                        return false;
                    }
                    else
                    {
                        if (targetUnboxed.GetType() == valueIInspectable)
                        {
                            // This means they are both valueIInspectable and we are comparing pointers.
                            // Can do direct comparison, so directly call into Store.
                            return ValueTypeInfo<valueIInspectable>::Store::Compare(targetUnboxed, sourceUnboxed);
                        }
                        else
                        {
                            // Both are not valueIInspectable, so compare them as raw values.
                            return targetUnboxed == sourceUnboxed;
                        }
                    }
                }
                else
                {
                    return false;
                }
            }

            template <>
            STATIC_SPEC bool Equals<valueTimeSpan>(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueTimeSpan);

                if (target.GetType() == valueTimeSpan)
                {
                    // Can do direct comparison, so directly call into Store.
                    return ValueTypeInfo<valueTimeSpan>::Store::Compare(target, source);
                }
                else
                {
                    return false;
                }
            }

            template <>
            STATIC_SPEC bool Equals<valueDateTime>(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                ASSERT(source.GetType() == valueDateTime);

                if (target.GetType() == valueDateTime)
                {
                    // Can do direct comparison, so directly call into Store.
                    return ValueTypeInfo<valueDateTime>::Store::Compare(target, source);
                }
                else
                {
                    return false;
                }
            }

            template <ValueType sourceValueType>
            static bool invoke(
                _In_ const CValue& target,
                _In_ const CValue& source)
            {
                return Equals<sourceValueType>(
                    target,
                    source);
            }
        };

        // Destruction handler.  Called only when value is owned.
        // It does not zero the underlying value in target.  ZeroState should be called immediately after calling Destroy.
        struct Destroy
        {
            template <ValueType sourceValueType>
            static void invoke(
                _Inout_ CValue& target,
                nullptr_t)
            {
                ValueTypeInfo<sourceValueType>::Store::Destroy(target);
            }
        };

        // Handler for checking if value type is an array.  Uses internal value type traits.
        struct IsArray
        {
            template <ValueType valueType>
            static constexpr bool invoke(
                _In_ const CValue& target,
                nullptr_t)
            {
                return is_array<valueType>::value;
            }
        };

        // Handler for checking if value type is a floating-point number.
        struct IsFloatingPoint
        {
            template <ValueType valueType>
            static constexpr bool invoke(
                _In_ const CValue& target,
                nullptr_t)
            {
                return std::is_floating_point<typename ValueTypeInfo<valueType>::Store::MappedType>::value;
            }
        };

        // Handler for checking if value type and instance is null.
        struct IsNullReference
        {
            template <ValueType valueType>
            using MappedType = typename ValueTypeInfo<valueType>::Store::MappedType;

            template <ValueType valueType>
            using StoredType = typename ValueTypeInfo<valueType>::Store::StoredType;

            struct tag_non_nullable {};
            struct tag_pointer {};
            struct tag_nullptr {};
            struct tag_type_handle {};
            struct tag_string {};

            template <ValueType valueType, typename = void>
            struct tag_provide
            {
                using type = tag_non_nullable;
            };

            template <ValueType valueType>
            struct tag_provide<valueType, typename std::enable_if<std::is_pointer<MappedType<valueType>>::value>::type>
            {
                using type = tag_pointer;
            };

            template <>
            struct tag_provide<valueNull>
            {
                using type = tag_nullptr;
            };

            template <>
            struct tag_provide<valueTypeHandle>
            {
                using type = tag_type_handle;
            };

            template <>
            struct tag_provide<valueString>
            {
                using type = tag_string;
            };

            template <ValueType valueType>
            static constexpr bool IsNullImpl(
                _In_ const CValue& target,
                tag_non_nullable)
            {
                return false;
            }

            template <ValueType valueType>
            static constexpr bool IsNullImpl(
                _In_ const CValue& target,
                tag_nullptr)
            {
                return true;
            }

            template <ValueType valueType>
            static bool IsNullImpl(
                _In_ const CValue& target,
                tag_pointer)
            {
                return ValueTypeInfo<valueType>::Store::Get(target) == ValueTypeInfo<valueType>::Empty;
            }

            template <ValueType valueType>
            static bool IsNullImpl(
                _In_ const CValue& target,
                tag_type_handle)
            {
                return ValueTypeInfo<valueType>::Store::Get(target) == KnownTypeIndex::UnknownType;
            }

            template <ValueType valueType>
            static bool IsNullImpl(
                _In_ const CValue& target,
                tag_string)
            {
                return ValueTypeInfo<valueType>::Store::Get(target).IsNull();
            }

            template <ValueType valueType>
            static constexpr bool invoke(
                _In_ const CValue& target,
                nullptr_t)
            {
                return IsNullImpl<valueType>(
                    target,
                    typename tag_provide<valueType>::type());
            }
        };
    };
}

CValue::CValue(
    _In_ const CValue& other)
{
    VERIFYHR(CopyConverted(other));
}

CValue::CValue(
    _Inout_ CValue&& other) noexcept
{
    *this = std::move(other);
}

CValue::~CValue()
{
    ReleaseAndReset();
}

CValue& CValue::operator=(
    _Inout_ CValue&& source) noexcept
{
    if (this != &source)
    {
        ReleaseAndReset();
        Dispatch<Handlers::Transfer<tag_move>, void>(source.GetType(), std::move(source));
        SetCustomData(source.GetCustomData());
        source.ZeroState();
    }

    return *this;
}

void CValue::WrapValue(
    _In_ const CValue& source)
{
    if (this != &source)
    {
        ReleaseAndReset();
        Dispatch<Handlers::Transfer<tag_shallow>, void>(source.GetType(), source);
        SetCustomData(source.GetCustomData());
    }
}

void CValue::CopyValue(
    _In_ const CValue& source)
{
    if (this != &source)
    {
        ReleaseAndReset();
        Dispatch<Handlers::Transfer<tag_deep>, void>(source.GetType(), source);
        SetCustomData(source.GetCustomData());
    }
}

_Check_return_ HRESULT CValue::CopyConverted(
    _In_ const CValue& source)
{
    if (this != &source)
    {
        ReleaseAndReset();
        IFC_RETURN((Dispatch<Handlers::ConvertAndCopy, HRESULT>(source.GetType(), source)));
        SetCustomData(source.GetCustomData());
    }

    return S_OK;
}

bool CValue::operator==(
    _In_ const CValue& other) const
{
    if (this != &other)
    {
        return Dispatch<Handlers::Compare, bool>(other.GetType(), other);
    }
    else
    {
        return true;
    }
}

bool CValue::operator!=(
    _In_ const CValue &other) const
{
    return !operator==(other);
}

void CValue::swap(
    _Inout_ CValue& other)
{
    if (this != &other)
    {
        std::swap(this->m_flags, other.m_flags);
        std::swap(this->m_value, other.m_value);
    }
}

bool CValue::OwnsValue() const
{
    return m_flags.m_state.m_ownsValue;
}

ValueType CValue::GetType() const
{
    return m_flags.m_state.m_type;
}

bool CValue::IsNull() const
{
    return Dispatch<Handlers::IsNullReference, bool>(GetType(), nullptr);
}

void CValue::SetNull()
{
    Set<valueNull>(nullptr);
}

bool CValue::IsUnset() const
{
    return GetType() == valueAny;
}

void CValue::Unset()
{
    Set<valueAny>(nullptr);
    SetOwnsValue(false);
}

bool CValue::IsNullOrUnset() const
{
    return IsUnset() || IsNull();
}

bool CValue::IsFloatingPoint() const
{
    return Dispatch<Handlers::IsFloatingPoint, bool>(GetType(), nullptr);
}

bool CValue::IsArray() const
{
    return Dispatch<Handlers::IsArray, bool>(GetType(), nullptr);
}

uint32_t CValue::GetArrayElementCount() const
{
    return (IsArray()) ? GetArrayElementCountInternal() : 0;
}

uint32_t CValue::GetArrayElementCountInternal() const
{
#if defined(_X86_) || defined(_ARM_)
    return m_value.m_array.m_count;
#elif defined(_AMD64_) || defined(_ARM64_)
    return m_flags.m_count;
#endif
}

void CValue::SetArrayElementCount(uint32_t count)
{
#if defined(_X86_) || defined(_ARM_)
    m_value.m_array.m_count = count;
#elif defined(_AMD64_) || defined(_ARM64_)
    m_flags.m_count = count;
#endif
}

void CValue::ReleaseAndReset()
{
    if (OwnsValue())
    {
        Dispatch<Handlers::Destroy, void>(GetType(), nullptr);
    }

    ZeroState();
}

void CValue::ZeroState()
{
    // Resets entire state of CValue to valueAny, value = 0 and flags to false.
    // m_asOne fields are unioned with state of CValue.

    m_value.m_asOne[0] = 0;
    m_value.m_asOne[1] = 0;

#if defined(_X86_) || defined(_ARM_)
    m_flags.m_asOne = 0;
#elif defined(_AMD64_) || defined(_ARM64_)
    m_flags.m_asOne[0] = 0;
    m_flags.m_asOne[1] = 0;
#endif
}

void CValue::SetType(
    ValueType valueType)
{
    m_flags.m_state.m_type = valueType;
}

void CValue::SetOwnsValue(
    bool ownsValue)
{
    m_flags.m_state.m_ownsValue = ownsValue;
}

uint32_t CValue::AsEnum() const
{
    return As<valueEnum>().m_value;
}

uint8_t CValue::AsEnum8() const
{
    return As<valueEnum8>().m_value;
}

_Check_return_ HRESULT CValue::GetEnum(
    _Out_ uint32_t& value) const
{
    ValueTypeInfo<valueEnum>::Store::MappedType storedValue = {};
    IFC_RETURN(Get<valueEnum>(storedValue));
    value = storedValue.m_value;
    return S_OK;
}

_Check_return_ HRESULT CValue::GetEnum(
    _Out_ uint32_t& value,
    _Out_ KnownTypeIndex& typeIndex) const
{
    ValueTypeInfo<valueEnum>::Store::MappedType storedValue = {};
    IFC_RETURN(Get<valueEnum>(storedValue));
    value = storedValue.m_value;
    typeIndex = storedValue.m_typeIndex;
    return S_OK;
}

void CValue::SetEnum(
    uint32_t value)
{
    Set<valueEnum>({ value, KnownTypeIndex::UnknownType });
}

void CValue::SetEnum(
    uint32_t value,
    KnownTypeIndex typeIndex)
{
    Set<valueEnum>({ value, typeIndex });
}

void CValue::SetEnum8(
    uint8_t value)
{
    Set<valueEnum8>({ value, KnownTypeIndex::UnknownType });
}

void CValue::SetEnum8(
    uint8_t value,
    KnownTypeIndex typeIndex)
{
    Set<valueEnum8>({ value, typeIndex });
}

xstring_ptr CValue::AsString() const
{
    return As<valueString>();
}

xencoded_string_ptr CValue::AsEncodedString() const
{
    return (GetType() == valueString) ?
        ValueStores::String<valueString>::Accessor::Get(GetField<valueString>(this->m_value)) :
        xencoded_string_ptr::NullString();
}

_Check_return_ HRESULT CValue::GetString(
    _Out_ xstring_ptr& value) const
{
    return Get<valueString>(value);
}

void CValue::SetString(
    _In_ const xstring_ptr& value)
{
    Set<valueString>(value);
}

_Check_return_ HRESULT CValue::SetString(
    HSTRING value)
{
    xstring_ptr valueAsxstr;
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(value, &valueAsxstr));
    SetString(std::move(valueAsxstr));
    return S_OK;
}

void CValue::SetString(
    _Inout_ xstring_ptr&& value)
{
    ReleaseAndReset();

    SetType(valueString);
    SetOwnsValue(true);

    ValueTypeInfo<valueString>::Store::MoveXString(
        *this,
        std::move(value));
}

xref_ptr<CDependencyObject> CValue::DetachObject()
{
    xref_ptr<CDependencyObject> obj;

    ASSERT(((GetType() == valueNull || GetType() == valueObject) && OwnsValue()) || GetType() == valueAny);

    if (GetType() == valueObject &&
        OwnsValue())
    {
        obj.attach(As<valueObject>());
        SetOwnsValue(false);
        ReleaseAndReset();
    }

    return obj;
}

CValueCustomData& CValue::GetCustomData()
{
    return m_flags.m_customData;
}

const CValueCustomData& CValue::GetCustomData() const
{
    return m_flags.m_customData;
}

void CValue::SetCustomData(
    const CValueCustomData& customData)
{
    m_flags.m_state.m_asOne &= ~CustomDataMask;
    m_flags.m_state.m_asOne |= (customData.m_asOne & CustomDataMask);
}

void CValue::SetEnumHelper(uint32_t value, KnownTypeIndex typeIndex)
{
    const CClassInfo* classInfo = DirectUI::MetadataAPI::GetClassInfoByIndex(typeIndex);

    if (classInfo->IsCompactEnum())
    {
        // This case happens when the enum is excluded from core:
        // T is an enum but not an enum class, and the values being passed
        // are small in magnitude but they're passed as 32 bits
        // e.g. defaultValue->Set(Windows::UI::Xaml::Controls::AppBarClosedDisplayMode_Hidden, KnownPropertyIndex::AppBar_ClosedDisplayMode);

        // Enum is 8 bits wide, but value being set is wider
        FAIL_FAST_ASSERT(value <= 0xffu);
        SetEnum8(static_cast<uint8_t>(value), typeIndex);
    }
    else
    {
        SetEnum(value, typeIndex);
    }
}

bool CValue::IsEnum() const
{
    return GetType() == valueEnum ||
        GetType() == valueEnum8;
}