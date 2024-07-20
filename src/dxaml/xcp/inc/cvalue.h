// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define CORE_E_INVALIDTYPE HRESULT(0x8000FFFFL)

#include <utility>
#include <CValueTypeInfo.h>
#include <CValueTraits.h>
#include <type_traits>

#ifndef EXP_CLANG
    // MSVC happily accepts static template specializations.
    #define STATIC_SPEC static
#else
    // Clang does not and emits a warning.
    #define STATIC_SPEC
#endif

namespace CValueDetails
{
    struct Handlers;

    // Number of bits allocated for ValueType.
    static constexpr uint16_t TypeBits = 6;

    static_assert(static_cast<uint32_t>(valueTypeSentinel) < (1 << TypeBits), "ValueType does not fit in allocated number of bits.");

    // Number of bits used by CValueState (ValueType + owns).
    // Update this if adding more CValue specific state (not custom data bits).
    static constexpr uint16_t CValueStateBits = TypeBits + 1;

    // Mask used to extract CValueCustomData bits from Flags union.
    static constexpr uint32_t CustomDataMask = (0xffffffff) << CValueStateBits;
}

class CValue;

// Auxiliary payload stored on CValue.
struct CValueCustomData
{
    operator uint32_t() const
    {
        return m_asOne & CValueDetails::CustomDataMask;
    }

    void SetIsIndependent(bool isIndependent)   { m_isIndependent = isIndependent; }
    bool IsIndependent() const                  { return m_isIndependent; }

    void SetIsSetLocally(bool isSetLocally)     { m_isSetLocally = isSetLocally; }
    bool IsSetLocally() const                   { return m_isSetLocally; }

    void SetIsSetByStyle(bool isSetByStyle)     { m_isSetByStyle = isSetByStyle; }
    bool IsSetByStyle() const                   { return m_isSetByStyle; }

private:
    friend class CValue;

    union
    {
        struct
        {
            // Anonymous member to demark bits used by CValueState (CValueState and CValueCustomData are in union).
            // Do not use, nor change here.
            uint32_t                 : CValueDetails::CValueStateBits;

            uint32_t m_isIndependent : 1;
            uint32_t m_isSetLocally  : 1;
            uint32_t m_isSetByStyle  : 1;
        };

        uint32_t m_asOne = 0;
    };
};

namespace CValueDetails
{
    union CValueState
    {
        struct
        {
            ValueType m_type         : TypeBits;
            uint32_t m_ownsValue     : 1;

            // Anonymous member to demark bits used by CValueCustomData (CValueState and CValueCustomData are in union).
            // Do not use, nor change here.
            uint32_t                 : 32 - CValueStateBits;
        };

        uint32_t m_asOne = 0;
    };

    // CValue control flags: internal state and custom data bits.
    union Flags
    {
        struct
        {
            // DWORD 0 - overlay CValue internal bits and custom bits

            union
            {
                CValueState m_state;
                CValueCustomData m_customData;
            };

            // DWORD 1 - only on x64, count of elements in array

#if defined(_AMD64_) || defined(_ARM64_)
            uint32_t m_count;
#endif
        };

#if defined(_X86_) || defined(_ARM_)
        uint32_t m_asOne = 0;
#elif defined(_AMD64_) || defined(_ARM64_)
        uint32_t m_asOne[2] = { 0, 0 };
#endif

    };

    // CValue value field.
    union Value
    {
        union
        {
            ValueTypeInfo<valueAny>::Store::StoredType m_any;
            ValueTypeInfo<valueNull>::Store::StoredType m_null;
            ValueTypeInfo<valueBool>::Store::StoredType m_bool;
            ValueTypeInfo<valueEnum>::Store::StoredType m_enum;
            ValueTypeInfo<valueSigned>::Store::StoredType m_signed;
            ValueTypeInfo<valueUnsigned>::Store::StoredType m_unsigned;
            ValueTypeInfo<valueInt64>::Store::StoredType m_int64;
            ValueTypeInfo<valueUInt64>::Store::StoredType m_uint64;
            ValueTypeInfo<valueFloat>::Store::StoredType m_float;
            ValueTypeInfo<valueDouble>::Store::StoredType m_double;
            ValueTypeInfo<valueString>::Store::StoredType m_string;
            ValueTypeInfo<valueColor>::Store::StoredType m_color;
            ValueTypeInfo<valuePoint>::Store::StoredType m_point;
            ValueTypeInfo<valueSize>::Store::StoredType m_size;
            ValueTypeInfo<valueRect>::Store::StoredType m_rect;
            ValueTypeInfo<valueThickness>::Store::StoredType m_thickness;
            ValueTypeInfo<valueGridLength>::Store::StoredType m_gridLength;
            ValueTypeInfo<valueCornerRadius>::Store::StoredType m_cornerRadius;
            ValueTypeInfo<valueDateTime>::Store::StoredType m_dateTime;
            ValueTypeInfo<valueTimeSpan>::Store::StoredType m_timeSpan;
            ValueTypeInfo<valueObject>::Store::StoredType m_object;
            ValueTypeInfo<valueInternalHandler>::Store::StoredType m_internalHandler;
            ValueTypeInfo<valueIUnknown>::Store::StoredType m_iunknown;
            ValueTypeInfo<valueIInspectable>::Store::StoredType m_iinspectable;
            ValueTypeInfo<valueTypeHandle>::Store::StoredType m_typeHandle;
            ValueTypeInfo<valueThemeResource>::Store::StoredType m_themeResource;
            ValueTypeInfo<valuePointer>::Store::StoredType m_pointer;
            ValueTypeInfo<valueVO>::Store::StoredType m_valueVO;
            ValueTypeInfo<valueTextRange>::Store::StoredType m_textRange;
            ValueTypeInfo<valueEnum8>::Store::StoredType m_enum8;
        };

        struct Array
        {
            union
            {
                ValueTypeInfo<valueSignedArray>::Store::StoredType m_signed;
                ValueTypeInfo<valueFloatArray>::Store::StoredType m_float;
                ValueTypeInfo<valueDoubleArray>::Store::StoredType m_double;
                ValueTypeInfo<valuePointArray>::Store::StoredType m_point;
            };

#if defined(_X86_) || defined(_ARM_)
            uint32_t m_count;
#endif
        }
        m_array;

        uint32_t m_asOne[2] = { 0, 0 };
    };

    template <ValueType valueType>
    static constexpr typename ValueTypeInfo<valueType>::Store::StoredType& GetField(_In_ Value& value);

#define DECLARE_STORAGE_FIELD_GETTER(typeEnum, field) \
    template <> STATIC_SPEC constexpr typename ValueTypeInfo<typeEnum>::Store::StoredType& GetField<typeEnum>(_In_ Value& value) { return field; }

    DECLARE_STORAGE_FIELD_GETTER(valueAny, value.m_any);
    DECLARE_STORAGE_FIELD_GETTER(valueNull, value.m_null);
    DECLARE_STORAGE_FIELD_GETTER(valueBool, value.m_bool);
    DECLARE_STORAGE_FIELD_GETTER(valueEnum, value.m_enum);
    DECLARE_STORAGE_FIELD_GETTER(valueSigned, value.m_signed);
    DECLARE_STORAGE_FIELD_GETTER(valueUnsigned, value.m_unsigned);
    DECLARE_STORAGE_FIELD_GETTER(valueInt64, value.m_int64);
    DECLARE_STORAGE_FIELD_GETTER(valueUInt64, value.m_uint64);
    DECLARE_STORAGE_FIELD_GETTER(valueFloat, value.m_float);
    DECLARE_STORAGE_FIELD_GETTER(valueDouble, value.m_double);
    DECLARE_STORAGE_FIELD_GETTER(valueSignedArray, value.m_array.m_signed);
    DECLARE_STORAGE_FIELD_GETTER(valueFloatArray, value.m_array.m_float);
    DECLARE_STORAGE_FIELD_GETTER(valueDoubleArray, value.m_array.m_double);
    DECLARE_STORAGE_FIELD_GETTER(valuePointArray, value.m_array.m_point);
    DECLARE_STORAGE_FIELD_GETTER(valueString, value.m_string);
    DECLARE_STORAGE_FIELD_GETTER(valueColor, value.m_color);
    DECLARE_STORAGE_FIELD_GETTER(valuePoint, value.m_point);
    DECLARE_STORAGE_FIELD_GETTER(valueSize, value.m_size);
    DECLARE_STORAGE_FIELD_GETTER(valueRect, value.m_rect);
    DECLARE_STORAGE_FIELD_GETTER(valueThickness, value.m_thickness);
    DECLARE_STORAGE_FIELD_GETTER(valueGridLength, value.m_gridLength);
    DECLARE_STORAGE_FIELD_GETTER(valueCornerRadius, value.m_cornerRadius);
    DECLARE_STORAGE_FIELD_GETTER(valueDateTime, value.m_dateTime);
    DECLARE_STORAGE_FIELD_GETTER(valueTimeSpan, value.m_timeSpan);
    DECLARE_STORAGE_FIELD_GETTER(valueObject, value.m_object);
    DECLARE_STORAGE_FIELD_GETTER(valueInternalHandler, value.m_internalHandler);
    DECLARE_STORAGE_FIELD_GETTER(valueIUnknown, value.m_iunknown);
    DECLARE_STORAGE_FIELD_GETTER(valueIInspectable, value.m_iinspectable);
    DECLARE_STORAGE_FIELD_GETTER(valueTypeHandle, value.m_typeHandle);
    DECLARE_STORAGE_FIELD_GETTER(valueThemeResource, value.m_themeResource);
    DECLARE_STORAGE_FIELD_GETTER(valuePointer, value.m_pointer);
    DECLARE_STORAGE_FIELD_GETTER(valueVO, value.m_valueVO);
    DECLARE_STORAGE_FIELD_GETTER(valueTextRange, value.m_textRange);
    DECLARE_STORAGE_FIELD_GETTER(valueEnum8, value.m_enum8);

#undef DECLARE_STORAGE_FIELD_GETTER

    template <ValueType valueType>
    static constexpr typename ValueTypeInfo<valueType>::Store::StoredType& GetField(const Value& value)
    {
        return GetField<valueType>(const_cast<Value&>(value));
    }
}

class CValue
{
    template <ValueType valueType>
    using ValueTypeInfo = CValueDetails::ValueTypeInfo<valueType>;

    template <ValueType valueType>
    using MappedType = typename ValueTypeInfo<valueType>::Store::MappedType;

    template <ValueType valueType>
    using ConstMappedType = typename ValueTypeInfo<valueType>::Store::ConstMappedType;

    template <size_t count>
    using tag_conversion_count = CValueDetails::tag_conversion_count<count>;

    template <ValueType valueType>
    using tag_value_type = CValueDetails::tag_value_type<valueType>;

    template <ValueType valueType>
    using ConversionSpec = CValueDetails::ConversionSpec<valueType>;

    // Non-const return type is by value.
    template <ValueType valueType>
    using ReturnType = MappedType<valueType>;

    // Default const return type is by value.
    template <ValueType valueType, typename = void>
    struct ConstReturnType_
    {
        using type = MappedType<valueType>;
    };

    // Const return type for pointers and references is by const-ref.
    template <ValueType valueType>
    struct ConstReturnType_<
        valueType,
        typename std::enable_if<std::is_pointer<MappedType<valueType>>::value || std::is_reference<MappedType<valueType>>::value>::type>
    {
        using type = ConstMappedType<valueType>;
    };

    // Const return type.
    template <ValueType valueType>
    using ConstReturnType = typename ConstReturnType_<valueType>::type;

public:
    CValue() = default;

    // Copy ctor makes a deep copy of value and unboxes CDependencyObject.
    CValue(
        _In_ const CValue& other);

    // Move ctor transfers value and ownership.
    // Difference from old CValue: leaves other in default constructed state (valueAny) as opposed to valueNull.
    CValue(
        _Inout_ CValue&& other) noexcept;

    // Releases value if it owns it.
    ~CValue();

    CValue& operator=(
        _In_ const CValue& source) = delete;

    // Move assignment operator transfers value and ownership.
    // Difference from old CValue: leaves other in default constructed state (valueAny) as opposed to valueNull.
    CValue& operator=(
        _Inout_ CValue&& source);

    // Equality comparison has the following rules:
    //   If value types are the same:
    //     Simple values - value comparison.
    //     IInspectables - try unboxing IPropertyValues and value comparison.
    //     References (e.g. point, size, thickness, etc.) - value comparison.
    //     Arrays - pointer comparison and sizes.
    //     Ref-counted objects - pointer comparison.
    //     String - string comparison.
    //   If value types are combination of valueFloat and valueDouble, compare as doubles.
    //   If value types are different, they are not equal.
    bool operator==(
        _In_ const CValue& other) const;

    // Inequality comparison, for comparison rules see operator==.
    bool operator!=(
        _In_ const CValue &other) const;

    // Swap contents of CValues.
    void swap(CValue& other);

    // Query whether CValue owns value.
    bool OwnsValue() const;

    // ValueType of this CValue.
    ValueType GetType() const;

    // Query if type contained is null.  It is true under following conditions:
    //   Is valueNull.
    //   Is a reference with null value.
    //   Is an array with null value.
    //   Is type handle with UnknownType value.
    //   Is string with NullString value.
    bool IsNull() const;

    // Resets to valueNull.
    void SetNull();

    // Returns true is value type is valueAny.
    bool IsUnset() const;

    // Resets to valueAny.
    void Unset();

    // Returns true if value is null or unset (see IsNull for details).
    bool IsNullOrUnset() const;

    // Returns true if value holds a floating point type (currently valueFloat or valueDouble) but not an array of floating-point numbers.
    bool IsFloatingPoint() const;

    // Returns true if value holds an array.
    bool IsArray() const;

    // Returns number of elements in array, or 0 if value does not hold an array type.
    uint32_t GetArrayElementCount() const;

    // Returns whether this is an enumeration value
    bool IsEnum() const;

    // Shallow copy of source.
    // Value types are copied, reference and array type pointers are copied but objects pointed to are not, reference counts are not incremented.
    // On destruction wrapped resource will not be released.
    void WrapValue(
        _In_ const CValue& source);

    // Deep copy of value held by source.
    // Value types are copied, reference and array type pointers are copied so are objects pointed to, reference counts are incremented.
    // On destruction copied resource will be released.
    // In new code please make a distinction whether your intent is just to make a copy, or copy unboxed value and call appropriate method.
    void CopyValue(
        _In_ const CValue& source);

    // Deep copy of value held by source.  If source contains CDependencyObject, it will be unboxed.
    // Value types are copied, reference and array type pointers are copied so are objects pointed to, reference counts are incremented.
    // On destruction copied resource will be released.
    // In new code please make a distinction whether your intent is just to make a copy, or copy unboxed value and call appropriate method.
    _Check_return_ HRESULT CopyConverted(
        _In_ const CValue& source);

    // Release held value if owned and reset state to valueAny.
    void ReleaseAndReset();

    // Returns instance of empty CValue.
    static CValue Empty()
    {
        return CValue();
    }

    // The new accessor interface.

    // As<*> methods will return a value regardless if CValue holds the type caller asked for.
    // If there is allowed conversion between types, a converted value will be returned.
    // If type does not match nor there is conversion, type's default value is returned.
    // For counted reference types (e.g. CDependencyObject, IInspectable, etc.) it does not add-ref.
    template <ValueType valueType>
    ReturnType<valueType> As()
    {
        ReturnType<valueType> result;

        if (TryGetValue<valueType>(result))
        {
            return result;
        }
        else
        {
            return ValueTypeInfo<valueType>::Empty;
        }
    }

    // As<*> methods will return a value regardless if CValue holds the type caller asked for.
    // If there is allowed conversion between types, a converted value will be returned.
    // If type does not match nor there is conversion, type's default value is returned.
    // For counted reference types (e.g. CDependencyObject, IInspectable, etc.) it does not add-ref.
    template <ValueType valueType>
    ConstReturnType<valueType> As() const
    {
        return const_cast<CValue*>(this)->As<valueType>();
    }

    // Get<*> methods will only return a value if CValue holds the correct type or there is allowed conversion.
    // In case it does not, default value is returned and returned result is error code.
    // For counted reference types (e.g. CDependencyObject, IInspectable, etc.) it does not add-ref.
    template <ValueType valueType>
    _Check_return_ HRESULT Get(
        _Out_ ReturnType<valueType>& value)
    {
        static_assert(CValueDetails::is_array<valueType>::value == false, "Do not call Get for array types.");

        if (TryGetValue<valueType>(value))
        {
            return S_OK;
        }
        else
        {
            value = ValueTypeInfo<valueType>::Empty;
            return CORE_E_INVALIDTYPE;
        }
    }

    // Get<*> methods will only return a value if CValue holds the correct type or there is allowed conversion.
    // In case it does not, default value is returned and returned result is error code.
    // For counted reference types (e.g. CDependencyObject, IInspectable, etc.) it does not add-ref.
    template <ValueType valueType>
    _Check_return_ HRESULT Get(
        _Out_ ConstReturnType<valueType>& value) const
    {
        return const_cast<CValue*>(this)->Get<valueType>(
            const_cast<ReturnType<valueType>&>(value));
    }

    // GetArray<*> methods will only return a value if CValue holds the correct type.
    // In case it does not, default value is returned and returned result is error code.
    template <ValueType valueType>
    _Check_return_ HRESULT GetArray(
        _Out_ uint32_t& count,
        _Out_ ReturnType<valueType>& value)
    {
        static_assert(CValueDetails::is_array<valueType>::value == true, "Do not call GetArray for non-array types.");

        if (GetType() == valueType)
        {
            count = GetArrayElementCountInternal();
            value = ValueTypeInfo<valueType>::Store::Get(*this);
            return S_OK;
        }
        else
        {
            value = ValueTypeInfo<valueType>::Empty;
            return CORE_E_INVALIDTYPE;
        }
    }

    // GetArray<*> methods will only return a value if CValue holds the correct type.
    // In case it does not, default value is returned and returned result is error code.
    template <ValueType valueType>
    _Check_return_ HRESULT GetArray(
        _Out_ uint32_t& count,
        _Out_ ConstReturnType<valueType>& value) const
    {
        return const_cast<CValue*>(this)->GetArray<valueType>(
            count,
            const_cast<ReturnType<valueType>&>(value));
    }

    // Set<*> releases previously held value and takes the new one.
    // For reference (e.g. XPOINTF, XSIZEF) and counted reference types (e.g. CDependencyObject, IInspectable, etc.)
    // it takes ownership, but does not copy nor add-ref.
    template <ValueType valueType>
    void Set(
        _In_ MappedType<valueType> value)
    {
        static_assert(CValueDetails::is_array<valueType>::value == false, "Do not call Set for array types.");

        ReleaseAndReset();

        SetType(valueType);
        SetOwnsValue(true);

        ValueTypeInfo<valueType>::Store::Set(*this, value);
    }

    // SetArray<*> releases previously held value and takes the new one.
    // It takes ownership, but does not copy.
    template <ValueType valueType>
    void SetArray(
        _In_ uint32_t count,
        _In_ MappedType<valueType> value)
    {
        static_assert(CValueDetails::is_array<valueType>::value == true, "Do not call SetArray for non-array types.");

        ReleaseAndReset();

        SetType(valueType);
        SetOwnsValue(true);
        SetArrayElementCount(count);

        ValueTypeInfo<valueType>::Store::Set(*this, value);
    }

    // SetAddRef<*> releases previously held value and takes the new one.
    // It takes ownership and add-refs.
    template <ValueType valueType>
    void SetAddRef(
        _In_ MappedType<valueType> value)
    {
        static_assert(CValueDetails::is_array<valueType>::value == false, "Do not call SetAddRef for array types.");
        static_assert(CValueDetails::is_ref_counted<valueType>::value == true, "Do not call SetAddRef for non-ref-counted types.");

        ReleaseAndReset();

        SetType(valueType);
        SetOwnsValue(true);

        ValueTypeInfo<valueType>::Store::Set(*this, value);
        ValueTypeInfo<valueType>::Store::AddRef(*this);
    }

    // Wrap<*> releases previously held value and takes the new one.
    // It does not take ownership, copy nor add-refs.
    template <ValueType valueType>
    void Wrap(
        _In_ MappedType<valueType> value)
    {
        static_assert(CValueDetails::is_array<valueType>::value == false, "Do not call Wrap for array types.");
        static_assert(CValueDetails::is_wrappable<valueType>::value == true, "Do not call Wrap for non-wrappable types.");

        ReleaseAndReset();

        SetType(valueType);
        SetOwnsValue(false);

        ValueTypeInfo<valueType>::Store::Set(*this, value);
    }

    // WrapArray<*> releases previously held value and takes the new one.
    // It does not take ownership nor copy.
    template <ValueType valueType>
    void WrapArray(
        _In_ uint32_t count,
        _In_ MappedType<valueType> value)
    {
        static_assert(CValueDetails::is_array<valueType>::value == true, "Do not call WrapArray for non-array types.");
        static_assert(CValueDetails::is_wrappable<valueType>::value == true, "Do not call WrapArray for non-wrappable types.");

        ReleaseAndReset();

        SetType(valueType);
        SetOwnsValue(false);
        SetArrayElementCount(count);

        ValueTypeInfo<valueType>::Store::Set(*this, value);
    }

    // Legacy interface.
    // Do not use in new code and opportunistically convert old code to use the new interface.
    // The new interface is const-correct and is easier to parametrize for templates.

    #define DECLARE_LEGACY_ACCESSORS(suffix) \
    ReturnType<value##suffix> As##suffix() const { return const_cast<CValue*>(this)->As<value##suffix>(); } \
    _Check_return_ HRESULT Get##suffix(_Out_ ReturnType<value##suffix>& value) const { return const_cast<CValue*>(this)->Get<value##suffix>(value); } \
    void Set##suffix(_In_ MappedType<value##suffix> value) { Set<value##suffix>(value); }

    #define DECLARE_LEGACY_ARRAY_ACCESSORS(suffix) \
    ReturnType<value##suffix> As##suffix() const { return const_cast<CValue*>(this)->As<value##suffix>(); } \
    _Check_return_ HRESULT Get##suffix(_Out_ ReturnType<value##suffix>& value, _Out_ uint32_t* count) const { return const_cast<CValue*>(this)->GetArray<value##suffix>(*count, value); } \
    void Set##suffix(uint32_t count, _In_ MappedType<value##suffix> value) { SetArray<value##suffix>(count, value); } \
    void Wrap##suffix(uint32_t count, _In_ ConstMappedType<value##suffix> value) { WrapArray<value##suffix>(count, const_cast<ReturnType<value##suffix>>(value)); }

    #define DECLARE_LEGACY_REFERENCE_ACCESSORS(suffix) \
    ReturnType<value##suffix> As##suffix() const { return const_cast<CValue*>(this)->As<value##suffix>(); } \
    _Check_return_ HRESULT Get##suffix(_Out_ ReturnType<value##suffix>& value) const { return const_cast<CValue*>(this)->Get<value##suffix>(value); } \
    void Set##suffix(_In_ MappedType<value##suffix> value) { Set<value##suffix>(value); } \
    void Wrap##suffix(_In_ ConstMappedType<value##suffix> value) { Wrap<value##suffix>(const_cast<ReturnType<value##suffix>>(value)); }

    #define DECLARE_LEGACY_COUNTED_REFERENCE_ACCESSORS(suffix) \
    ReturnType<value##suffix> As##suffix() const { return const_cast<CValue*>(this)->As<value##suffix>(); } \
    _Check_return_ HRESULT Get##suffix(_Out_ ReturnType<value##suffix>& value) const { return const_cast<CValue*>(this)->Get<value##suffix>(value); } \
    void Set##suffix##NoRef(_In_ MappedType<value##suffix> value) { Set<value##suffix>(value); } \
    void Set##suffix##AddRef(_In_ MappedType<value##suffix> value) { SetAddRef<value##suffix>(value); } \
    void Wrap##suffix##NoRef(_In_ ConstMappedType<value##suffix> value) { Wrap<value##suffix>(const_cast<ReturnType<value##suffix>>(value)); }

    // DO NOT ADD LEGACY ACCESSORS FOR NEW TYPES, USE NEW INTERFACE IN NEW CODE!

    DECLARE_LEGACY_ACCESSORS(Bool);
    DECLARE_LEGACY_ACCESSORS(Signed);
    DECLARE_LEGACY_ACCESSORS(Unsigned);
    DECLARE_LEGACY_ACCESSORS(Int64);
    DECLARE_LEGACY_ACCESSORS(UInt64);
    DECLARE_LEGACY_ACCESSORS(Float);
    DECLARE_LEGACY_ACCESSORS(Double);
    DECLARE_LEGACY_ACCESSORS(Color);
    DECLARE_LEGACY_ACCESSORS(DateTime);
    DECLARE_LEGACY_ACCESSORS(TimeSpan);
    DECLARE_LEGACY_ACCESSORS(InternalHandler);
    DECLARE_LEGACY_ACCESSORS(TypeHandle);
    DECLARE_LEGACY_ACCESSORS(Pointer);
    DECLARE_LEGACY_ARRAY_ACCESSORS(SignedArray);
    DECLARE_LEGACY_ARRAY_ACCESSORS(FloatArray);
    DECLARE_LEGACY_ARRAY_ACCESSORS(DoubleArray);
    DECLARE_LEGACY_ARRAY_ACCESSORS(PointArray);
    DECLARE_LEGACY_REFERENCE_ACCESSORS(Point);
    DECLARE_LEGACY_REFERENCE_ACCESSORS(Size);
    DECLARE_LEGACY_REFERENCE_ACCESSORS(Rect);
    DECLARE_LEGACY_REFERENCE_ACCESSORS(Thickness);
    DECLARE_LEGACY_REFERENCE_ACCESSORS(GridLength);
    DECLARE_LEGACY_REFERENCE_ACCESSORS(CornerRadius);
    DECLARE_LEGACY_COUNTED_REFERENCE_ACCESSORS(Object);
    DECLARE_LEGACY_COUNTED_REFERENCE_ACCESSORS(IUnknown);
    DECLARE_LEGACY_COUNTED_REFERENCE_ACCESSORS(IInspectable);
    DECLARE_LEGACY_COUNTED_REFERENCE_ACCESSORS(ThemeResource);

    // DO NOT ADD LEGACY ACCESSORS FOR NEW TYPES, USE NEW INTERFACE IN NEW CODE!

    #undef DECLARE_LEGACY_ACCESSORS
    #undef DECLARE_LEGACY_ARRAY_ACCESSORS
    #undef DECLARE_LEGACY_REFERENCE_ACCESSORS
    #undef DECLARE_LEGACY_COUNTED_REFERENCE_ACCESSORS

    // enum

    // Returns enumeration value regardless if CValue holds valueEnum.
    // If CValue does not hold valueEnum, type's default value is returned.
    uint32_t AsEnum() const;
    uint8_t AsEnum8() const;

    // Returns enumeration value if CValue holds the correct type.
    // In case it does not, default value is returned and returned result is error code.
    _Check_return_ HRESULT GetEnum(
        _Out_ uint32_t& value) const;

    // Returns enumeration value and type index if CValue holds the correct type.
    // In case it does not, default value is returned and returned result is error code.
    _Check_return_ HRESULT GetEnum(
        _Out_ uint32_t& value,
        _Out_ KnownTypeIndex& typeIndex) const;

    // Releases previously held value and sets the value to one passed.  KnownTypeIndex is set to UnknownType.
    void SetEnum(
        uint32_t value);

    // Releases previously held value and sets the value to one passed.  KnownTypeIndex is set to typeIndex.
    void SetEnum(
        uint32_t value,
        KnownTypeIndex typeIndex);

    // Releases previously held value and sets the value to one passed.  KnownTypeIndex is set to UnknownType.
    void SetEnum8(
        uint8_t value);

    // Releases previously held value and sets the value to one passed.  KnownTypeIndex is set to typeIndex.
    void SetEnum8(
        uint8_t value,
        KnownTypeIndex typeIndex);

    template <typename T>
    typename std::enable_if_t<std::is_enum_v<T> && sizeof(T) == 1> SetEnum(
        T value)
    {
        SetEnum8(static_cast<uint8_t>(value));
    }

    template <typename T>
    typename std::enable_if_t<std::is_enum_v<T> && sizeof(T) == 1> SetEnum(
        T value,
        KnownTypeIndex typeIndex)
    {
        SetEnum8(static_cast<uint8_t>(value), typeIndex);
    }

    template <typename T>
    typename std::enable_if_t<std::is_enum_v<T> && sizeof(T) != 1> SetEnum(
        T value)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t), "Max supported size for enum is 32 bits.");
        SetEnum(static_cast<uint32_t>(value));
    }
    
    template <typename T>
    typename std::enable_if_t<std::is_enum_v<T> && sizeof(T) != 1> SetEnum(
        T value,
        KnownTypeIndex typeIndex)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t), "Max supported size for enum is 32 bits.");
        SetEnum(static_cast<uint32_t>(value), typeIndex);
    }

    template <typename T>
    typename std::enable_if_t<std::is_enum_v<T>> Set(
        T value)
    {
        SetEnum(value);
    }

    template <typename T>
    void Set(
        T value,
        KnownTypeIndex typeIndex)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t), "Max supported size for enum is 32 bits.");
        SetEnumHelper(static_cast<uint32_t>(value), typeIndex);
    }
    
    // string

    // Returns string value regardless if CValue holds valueEnum.
    // If CValue does not hold valueString, type's default value is returned.
    xstring_ptr AsString() const;

    // Returns undecoded xencoded_string_ptr value regardless if CValue holds valueEnum.
    // If CValue does not hold valueString, type's default value is returned.
    xencoded_string_ptr AsEncodedString() const;

    // Returns string value regardless if CValue holds the correct type.
    // In case it does not, default value is returned and returned result is error code.
    _Check_return_ HRESULT GetString(
        _Out_ xstring_ptr& value) const;

    // Releases previously held value and sets the value to one passed.
    void SetString(
        _In_ const xstring_ptr& value);

    // Releases previously held value, converts HSTRING to xstring_ptr and sets the value.
    _Check_return_ HRESULT SetString(
        _In_ HSTRING value);

    // Releases previously held value, moves xstring_ptr to this CValue.
    void SetString(
        _Inout_ xstring_ptr&& value);

    // If value held is valueObject and CValue owns it, reference is moved and returned in xref_ptr and this CValue is reset.
    xref_ptr<CDependencyObject> DetachObject();

    // Custom data bits access

    CValueCustomData& GetCustomData();

    const CValueCustomData& GetCustomData() const;

    void SetCustomData(
        _In_ const CValueCustomData& customData);

private:
    friend struct CValueDetails::Handlers;
    friend struct CValueDetails::ValueStores;

    template <ValueType valueType>
    friend struct CValueDetails::ValueTypeInfo;

    template <typename Handler, typename OutType, typename V>
    OutType Dispatch(
        _Inout_ ValueType valueType,
        _Inout_ V&& arg);

    template <typename Handler, typename OutType, typename V>
    OutType Dispatch(
        _Inout_ ValueType valueType,
        _Inout_ V&& arg) const
    {
        return const_cast<CValue*>(this)->Dispatch<Handler, OutType, V>(valueType, std::forward<V>(arg));
    }

    // No conversion value getter specialization.
    template <ValueType valueType>
    bool TryGetValueImpl(
        _Out_ ReturnType<valueType>& result,
        tag_conversion_count<0>) const
    {
        if (GetType() == valueType)
        {
            result = ValueTypeInfo<valueType>::Store::Get(*this);
            return true;
        }
        else
        {
            return false;
        }
    }

    // One conversion value getter specialization.
    template <ValueType valueType>
    bool TryGetValueImpl(
        _Out_ ReturnType<valueType>& result,
        tag_conversion_count<1>) const
    {
        constexpr ValueType vt1 = ConversionSpec<valueType>::conversions::template get<0>();

        switch (GetType())
        {
            case valueType:
                return TryGetValueImpl<valueType>(
                    result,
                    tag_conversion_count<0>());

            case vt1:
                result = ConversionSpec<valueType>::Convert(
                    ValueTypeInfo<vt1>::Store::Get(*this),
                    tag_value_type<vt1>());
                return true;
        }

        return false;
    }

    // Two conversions value getter specialization.
    template <ValueType valueType>
    bool TryGetValueImpl(
        _Out_ ReturnType<valueType>& result,
        tag_conversion_count<2>) const
    {
        constexpr ValueType vt1 = ConversionSpec<valueType>::conversions::template get<0>();
        constexpr ValueType vt2 = ConversionSpec<valueType>::conversions::template get<1>();

        switch (GetType())
        {
            case valueType:
                return TryGetValueImpl<valueType>(
                    result,
                    tag_conversion_count<0>());

            case vt1:
                result = ConversionSpec<valueType>::Convert(
                    ValueTypeInfo<vt1>::Store::Get(*this),
                    tag_value_type<vt1>());
                return true;

            case vt2:
                result = ConversionSpec<valueType>::Convert(
                    ValueTypeInfo<vt2>::Store::Get(*this),
                    tag_value_type<vt2>());
                return true;
        }

        return false;
    }

    // Dispatches to a correct method based on the number of allowed conversions.
    template <ValueType valueType>
    bool TryGetValue(
        _Out_ ReturnType<valueType>& result) const
    {
        return TryGetValueImpl<valueType>(
            result,
            tag_conversion_count<ConversionSpec<valueType>::conversions::count>());
    }

    // Zeros state flags and value.  Does not release held resource.
    void ZeroState();

    // Returns element count from architecture-specific layout.  Does not check if held type is an array.
    uint32_t GetArrayElementCountInternal() const;

    // Sets element count in architecture-specific layout.  Does not check if held type is an array.
    void SetArrayElementCount(
        uint32_t count);

    void SetType(
        ValueType valueType);

    void SetOwnsValue(
        bool ownsValue);

    void SetEnumHelper(
        uint32_t value,
        KnownTypeIndex typeIndex);

    CValueDetails::Value m_value {};
    CValueDetails::Flags m_flags {};
};

#if defined(_X86_) || defined(_ARM_)

static_assert(sizeof(CValueDetails::Flags) <= 4, "Size of CValueDetails::Flags > 4.");
static_assert(alignof(CValueDetails::Flags) <= 4, "Alignment of CValueDetails::Flags > 4.");

static_assert(sizeof(CValueDetails::Value) <= 8, "Size of CValueDetails::Value > 8.");
static_assert(alignof(CValueDetails::Value) <= 4, "Alignment of CValueDetails::Value > 4.");

static_assert(sizeof(CValue) <= 12, "Size of CValue > 12.");
static_assert(alignof(CValue) <= 4, "Alignment of CValue > 4.");

#elif defined(_AMD64_) || defined(_ARM64_)

static_assert(sizeof(CValueDetails::Flags) <= 8, "Size of CValueDetails::Flags > 8.");
static_assert(alignof(CValueDetails::Flags) <= 4, "Alignment of CValueDetails::Flags > 4.");

static_assert(sizeof(CValueDetails::Value) <= 8, "Size of CValueDetails::Value > 8.");
static_assert(alignof(CValueDetails::Value) <= 4, "Alignment of CValueDetails::Value > 4.");

static_assert(sizeof(CValue) <= 16, "Size of CValue > 16.");
static_assert(alignof(CValue) <= 4, "Alignment of CValue > 4.");

#else

static_assert(false, "Unknown platform.");

#endif