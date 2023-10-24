// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Because of the current header configuration on the Macintosh, it is
// difficult to get the definition of E_INVALIDARG loaded here.
// CORE_E_INVALIDTYPE has the same value as E_UNEXPECTED.
#define CORE_E_INVALIDTYPE HRESULT(0x8000FFFFL)

#include <InternalEventHandler.h>
#include <xstring_ptr.h>
#include <ValueType.h>
#include <Indexes.g.h>
#include <xref_ptr.h>
#include <ComPtr.h>

class CThemeResource;
class CDependencyObject;
class CValue;

#ifndef MAX_VALUE_COUNT
#define MAX_VALUE_COUNT 16777215
#endif

namespace Automation
{
    struct CValueEnum
    {
        UINT32 nValue;
        KnownTypeIndex eTypeIndex;
    };

    // Definition of a lightweight container for property values.
    class CValue
    {
        friend class ::CDependencyObject;

    public:
        CValue()
            : CountTypeAndFlags(0)
            , m_lValue(0)
        {}

        CValue(const CValue& other)
            : CValue()
        {
            (*this) = other;
        }

        CValue(CValue&& other)
        {
            CountTypeAndFlags = other.EncodeCountTypeAndFlags();
            other.ResetCountTypeAndFlags();

            m_lValue = other.m_lValue;
            other.m_lValue = 0;
        }

        ~CValue();

        // move assignment operator
        CValue& operator=(CValue&& other)
        {
            if (this != &other)
            {
                std::swap(CountTypeAndFlags, other.CountTypeAndFlags);
                std::swap(m_lValue, other.m_lValue);
                other.ReleaseAndReset();
            }

            return (*this);
        }

        // '==' operator.
        // Returns shallow equality for reference types and arrays.
        // Returns equality for simple types (Float, Signed, Bool, Enum, Color, Double), structs such as Size, Point, CornerRadius, Thickness, Rect, GridLength and String
        bool operator==(const CValue &other) const;

        bool operator!=(const CValue &other) const
        {
            return !operator==(other);
        }

        void swap(CValue& other)
        {
            if (this != &other)
            {
                std::swap(CountTypeAndFlags, other.CountTypeAndFlags);
                std::swap(m_lValue, other.m_lValue);
            }
        }

        static CValue Empty()
        {
            return CValue();
        }

        bool IsFloatingPoint() const
        {
            return (m_type == valueFloat || m_type == valueDouble);
        }

        bool IsEnum() const
        {
            return (m_type == valueEnum || m_type == valueEnum8);
        }

        void SetFloat(_In_ XFLOAT val) { ResetValue(valueFloat, 0); m_eValue = val; }
        void SetSigned(_In_ XINT32 val) { ResetValue(valueSigned, 0); m_iValue = val; }
        void SetInt64(_In_ INT64 val) { ResetValue(valueInt64, 0); m_lValue = val; }
        void SetBool(_In_ bool val) { ResetValue(valueBool, 0); m_nValue = (val ? TRUE : FALSE); }
        void SetColor(_In_ XUINT32 val) { ResetValue(valueColor, 0); m_nValue = val; }
        void SetInternalHandler(_In_opt_ INTERNAL_EVENT_HANDLER val) { ResetValue(valueInternalHandler, 0); m_pHandler = val; }
        void SetNull() { ResetValue(valueNull, 0); m_dValue = 0; }
        void Unset() { ResetValue(valueAny, 0, FALSE); m_dValue = 0; }
        void SetPointer(_In_ void *val) { ResetValue(valuePointer, 0); m_pvValue = val; }
        void SetTimeSpan(_In_ wf::TimeSpan val) { ResetValue(valueTimeSpan, 0); m_lValue = val.Duration; }
        void SetDateTime(_In_ wf::DateTime val) { ResetValue(valueDateTime, 0); m_lValue = val.UniversalTime; }
        void SetTypeHandle(_In_ KnownTypeIndex val) { ResetValue(valueTypeHandle, 0); m_tValue = val; }

        void SetEnum(_In_ XUINT32 val)
        {
            ResetValue(valueEnum, 0);
            m_enumValue.nValue = val;
            m_enumValue.eTypeIndex = KnownTypeIndex::UnknownType;
        }

        void SetEnum(_In_ XUINT32 val, _In_ KnownTypeIndex eEnumTypeIndex)
        {
            ResetValue(valueEnum, 0);
            m_enumValue.nValue = val;
            m_enumValue.eTypeIndex = eEnumTypeIndex;
        }

        void SetEnum8(_In_ uint8_t val)
        {
            ResetValue(valueEnum8, 0);
            m_enumValue.nValue = val;
            m_enumValue.eTypeIndex = KnownTypeIndex::UnknownType;
        }

        void SetEnum8(_In_ uint8_t val, _In_ KnownTypeIndex eEnumTypeIndex)
        {
            ResetValue(valueEnum8, 0);
            m_enumValue.nValue = val;
            m_enumValue.eTypeIndex = eEnumTypeIndex;
        }

        void WrapIInspectableNoRef(_In_opt_ IInspectable* pInstance)
        {
            ResetValue(valueIInspectable, 0, /* fOwnsValue */ FALSE);
            m_pInspValue = pInstance;
        }

        void SetIInspectableNoRef(_In_opt_ IInspectable* pInstance)
        {
            ResetValue(valueIInspectable, 0);
            m_pInspValue = pInstance;
        }

        void SetIInspectableAddRef(_In_opt_ IInspectable* pInstance)
        {
            ResetValue(valueIInspectable, 0);
            SetInterface(m_pInspValue, pInstance);
        }

        void WrapObjectNoRef(_In_opt_ CDependencyObject *val)
        {
            ResetValue(valueObject, 0, /* fOwnsValue */ FALSE);
            m_pdoValue = val;
        }

        void SetObjectNoRef(_In_opt_ CDependencyObject *val)
        {
            ResetValue(valueObject, 0);
            m_pdoValue = val;
        }

        void SetObjectAddRef(_In_opt_ CDependencyObject *val);

        void WrapIUnknownNoRef(_In_opt_ IUnknown* pInstance)
        {
            ResetValue(valueIUnknown, 0, /* fOwnsValue */ FALSE);
            m_pUnkValue = pInstance;
        }

        void SetIUnknownNoRef(_In_ IUnknown* pValue)
        {
            ResetValue(valueIUnknown, 0, /* fOwnsValue */ FALSE);
            m_pUnkValue = pValue;
        }

        void SetIUnknownAddRef(_In_ IUnknown* pValue)
        {
            ResetValue(valueIUnknown, 0);
            SetInterface(m_pUnkValue, pValue);
        }

        // The following take a potentially user defined count and we need to protect
        // against buffer over runs and bogus dereferences.
        void SetString(_In_ const xstring_ptr& val)
        {
            ResetValue(valueString, 0 /* count can be obtained from xencoded_string_ptr */);
            m_estrValue = xstring_ptr::Encode(val);
        }

        void SetString(_Inout_ xstring_ptr&& val)
        {
            ResetValue(valueString, 0 /* count can be obtained from xencoded_string_ptr */);
            m_estrValue = xstring_ptr::MoveEncode(std::forward<xstring_ptr>(val));
        }

        _Check_return_ HRESULT SetString(_In_ HSTRING val);

        void SetPoint(_In_ XPOINTF *val)
        {
            ResetValue(valuePoint, 2);
            m_peValue = (XFLOAT *)val;
        }

        void WrapPoint(_In_ const XPOINTF *val)
        {
            ResetValue(valuePoint, 2, /* fOwnsValue */ FALSE);
            m_peValue = (XFLOAT *)val;
        }

        void SetDouble(_In_ DOUBLE val)
        {
            ResetValue(valueDouble, 1);
            m_dValue = val;
        }

        void SetRect(_In_ XRECTF *val)
        {
            ResetValue(valueRect, 4);
            m_peValue = (XFLOAT *)val;
        }

        void WrapRect(_In_ const XRECTF *val)
        {
            ResetValue(valueRect, 4, /* fOwnsValue */ FALSE);
            m_peValue = (XFLOAT *)val;
        }

        void SetThickness(_In_ XTHICKNESS *val)
        {
            ResetValue(valueThickness, 4);
            m_peValue = (XFLOAT *)val;
        }

        void WrapThickness(_In_ const XTHICKNESS *val)
        {
            ResetValue(valueThickness, 4, /* fOwnsValue */ FALSE);
            m_peValue = (XFLOAT *)val;
        }

        void SetCornerRadius(_In_ XCORNERRADIUS *val)
        {
            ResetValue(valueCornerRadius, 4);
            m_peValue = (XFLOAT *)val;
        }

        void WrapCornerRadius(_In_ const XCORNERRADIUS *val)
        {
            ResetValue(valueCornerRadius, 4, /* fOwnsValue */ FALSE);
            m_peValue = (XFLOAT *)val;
        }

        void SetSize(_In_ XSIZEF *val)
        {
            ResetValue(valueSize, 2);
            m_peValue = (XFLOAT *)val;
        }

        void WrapSize(_In_ const XSIZEF *val)
        {
            ResetValue(valueSize, 2, /* fOwnsValue */ FALSE);
            m_peValue = (XFLOAT *)val;
        }

        void SetGridLength(_In_ XGRIDLENGTH *val)
        {
            ResetValue(valueGridLength, 2);
            m_peByteValue = (XUINT8*)val;
        }

        void WrapGridLength(_In_ const XGRIDLENGTH *val)
        {
            ResetValue(valueGridLength, 2, /* fOwnsValue */ FALSE);
            m_peByteValue = (XUINT8*)val;
        }

        void SetFloatArray(_In_ XUINT32 cnt, _In_ XFLOAT *pe)
        {
            ResetValue(valueFloatArray, cnt > MAX_VALUE_COUNT ? MAX_VALUE_COUNT : cnt);
            m_peValue = m_internalCount ? pe : nullptr;
        }

        void WrapFloatArray(_In_ XUINT32 cnt, _In_ XFLOAT *pe)
        {
            ResetValue(valueFloatArray, cnt > MAX_VALUE_COUNT ? MAX_VALUE_COUNT : cnt, /* fOwnsValue */ FALSE);
            m_peValue = m_internalCount ? pe : nullptr;
        }

        void SetDoubleArray(_In_ XUINT32 cnt, _In_ XDOUBLE *pd)
        {
            ResetValue(valueDoubleArray, cnt > MAX_VALUE_COUNT ? MAX_VALUE_COUNT : cnt);
            m_pdfValue = m_internalCount ? pd : nullptr;
        }

        void WrapDoubleArray(_In_ XUINT32 cnt, _In_ XDOUBLE *pd)
        {
            ResetValue(valueDoubleArray, cnt > MAX_VALUE_COUNT ? MAX_VALUE_COUNT : cnt, FALSE);
            m_pdfValue = m_internalCount ? pd : nullptr;
        }

        void SetSignedArray(_In_ XUINT32 cnt, _In_ XINT32 *pi)
        {
            ResetValue(valueSignedArray, cnt > MAX_VALUE_COUNT ? MAX_VALUE_COUNT : cnt);
            m_piValue = m_internalCount ? pi : nullptr;
        }

        void WrapSignedArray(_In_ XUINT32 cnt, _In_ XINT32 *pi)
        {
            ResetValue(valueSignedArray, cnt > MAX_VALUE_COUNT ? MAX_VALUE_COUNT : cnt, FALSE);
            m_piValue = m_internalCount ? pi : nullptr;
        }

        void SetPointArray(_In_ XUINT32 cnt, _In_ XPOINTF *pe)
        {
            ResetValue(valuePointArray, cnt > MAX_VALUE_COUNT ? MAX_VALUE_COUNT : cnt);
            m_peValue = m_internalCount ? reinterpret_cast<XFLOAT*>(pe) : nullptr;
        }

        void WrapPointArray(_In_ const XUINT32 cnt, _In_ XFLOAT *pe)
        {
            ResetValue(valuePointArray, cnt > MAX_VALUE_COUNT ? MAX_VALUE_COUNT : cnt, /* fOwnsValue */ FALSE);
            m_peValue = m_internalCount ? pe : nullptr;
        }

        void SetThemeResourceNoRef(_In_ CThemeResource* resource)
        {
            ResetValue(valueThemeResource, 0);
            m_pThemeResource = resource;
        }

        void SetThemeResourceAddRef(_In_ CThemeResource* resource);

        void WrapThemeResourceNoRef(_In_ CThemeResource* resource)
        {
            ResetValue(valueThemeResource, 0, /* fOwnsValue */ FALSE);
            m_pThemeResource = resource;
        }

        CThemeResource* AsThemeResource() const
        {
            return (m_type == valueThemeResource) ? m_pThemeResource : nullptr;
        }

        void WrapValue(_In_ const CValue& value);

        void *GetValuePointer() const
        {
            switch (m_type)
            {
                case valueFloat:
                    return const_cast<XFLOAT*>(&m_eValue);

                case valueSigned:
                    return const_cast<XINT32*>(&m_iValue);

                case valueInt64:
                    return const_cast<INT64*>(&m_lValue);

                case valueBool:
                case valueEnum:
                case valueColor:
                    return const_cast<uint32_t*>(&m_nValue);
                case valueEnum8:
                    return reinterpret_cast<uint8_t*>(const_cast<uint32_t*>(&m_nValue));
                case valueNull:
                case valueAny:
                    return nullptr;

                case valueString:
                case valuePoint:
                case valueRect:
                case valueFloatArray:
                case valueObject:
                case valueThickness:
                case valueCornerRadius:
                case valueInternalHandler:
                case valueSize:
                case valueGridLength:
                case valuePointArray:
                case valueDouble:
                case valueIUnknown:
                case valuePointer:
                    return m_pvValue;
                case valueSignedArray:
                    return m_piValue;
                case valueDoubleArray:
                    return m_pdfValue;
            }

            return nullptr;
        }

        void ReleaseAndReset();

        void SetIsIndependent(bool isIndependent) { m_isIndependent = isIndependent; }
        bool IsIndependent() const { return m_isIndependent; }

        // Copy value of the given source to this CValue instance.  If the source
        //  object holds on to something else, either make a copy (for allocated
        //  memory like strings) or add a reference (to reference-counted object).
        _Check_return_ HRESULT CopyConverted(_In_ const CValue& source);

        _Check_return_ HRESULT ConvertFrom(_In_ const ::CValue& source);
        _Check_return_ HRESULT ConvertTo(_Out_::CValue& dest) const;

        KnownTypeIndex GetTypeIndex() const;

        //-----------------------------------------------------------------------------------------------------------------------------
        //  Safe getters that verify a value's type before returning the actual value.  There are two flavors, As...() and Get...().
        //  As...() will return the actual value or zero/nullptr if the types don't match, convenient for inline calls.
        //  Get...() sets the value and returns an HRESULT, convenient for IFC() checking.
        //-----------------------------------------------------------------------------------------------------------------------------
        XFLOAT AsFloat() const
        {
            return m_type == valueFloat ? m_eValue : 0.0f;
        }
        _Check_return_ HRESULT GetFloat(XFLOAT& val) const
        {
            val = AsFloat();
            return m_type == valueFloat ? S_OK : CORE_E_INVALIDTYPE;
        }

        DOUBLE AsDouble() const
        {
            if (m_type == valueFloat)
            {
                return m_eValue;
            }
            return m_type == valueDouble ? m_dValue : 0.0f;
        }
        _Check_return_ HRESULT GetDouble(XDOUBLE& val) const
        {
            val = AsDouble();
            return m_type == valueFloat || m_type == valueDouble ? S_OK : CORE_E_INVALIDTYPE;
        }

        XINT32 AsSigned() const
        {
            return m_type == valueSigned ? m_iValue : 0;
        }
        _Check_return_ HRESULT GetSigned(XINT32& val) const
        {
            val = AsSigned();
            return m_type == valueSigned ? S_OK : CORE_E_INVALIDTYPE;
        }

        INT64 AsInt64() const
        {
            return m_type == valueInt64 ? m_lValue : 0;
        }
        _Check_return_ HRESULT GetInt64(INT64& val) const
        {
            val = AsInt64();
            return m_type == valueInt64 ? S_OK : CORE_E_INVALIDTYPE;
        }

        KnownTypeIndex AsTypeHandle() const
        {
            return m_type == valueTypeHandle ? m_tValue : KnownTypeIndex::UnknownType;
        }

        bool AsBool() const
        {
            //
            // Using (m_nValue != 0) to ensure that the type is compatible
            // with bool. Otherwise CE compilers will complain for incompatibility
            // between bool and bool. With this, compiler will automatically do the casting
            //
            return m_type == valueBool ? (m_nValue != 0) : false;
        }

        _Check_return_ HRESULT GetBool(bool& val) const
        {
            //
            // Using (AsBool() != FALSE) to ensure that the type is compatible
            // with bool. Otherwise CE compilers will complain for incompatibility
            // between bool and bool. With this, compiler will automatically do the casting
            //
            val = AsBool();
            return m_type == valueBool ? S_OK : CORE_E_INVALIDTYPE;
        }

        XUINT32 AsEnum() const
        {
            return IsEnum() ? m_enumValue.nValue : 0;
        }

        _Check_return_ HRESULT GetEnum(XUINT32& val) const
        {
            val = AsEnum();
            return (IsEnum()) ? S_OK : CORE_E_INVALIDTYPE;
        }

        _Check_return_ HRESULT GetEnum(XUINT32& val, KnownTypeIndex& typeIndex) const
        {
            val = AsEnum();
            typeIndex = m_type == valueEnum ? m_enumValue.eTypeIndex : KnownTypeIndex::UnknownType;
            return IsEnum() ? S_OK : CORE_E_INVALIDTYPE;
        }

        XUINT32 AsColor() const
        {
            return m_type == valueColor ? m_nValue : 0;
        }
        _Check_return_ HRESULT GetColor(XUINT32& val) const
        {
            val = AsColor();
            return m_type == valueColor ? S_OK : CORE_E_INVALIDTYPE;
        }

        void AsEphemeralString(_Out_ xephemeral_string_ptr& val) const
        {
            xephemeral_string_ptr::Decode(
                valueString == m_type ? m_estrValue : xencoded_string_ptr::NullString(),
                &val);
        }

        xstring_ptr AsString() const
        {
            if (valueString == m_type)
            {
                return xstring_ptr::Decode(m_estrValue);
            }
            else
            {
                return xstring_ptr::NullString();
            }
        }
        _Check_return_ HRESULT GetString(_Out_ xstring_ptr* pstrVal) const
        {
            if (valueString == m_type)
            {
                *pstrVal = xstring_ptr::Decode(m_estrValue);
                return S_OK;
            }
            else
            {
                pstrVal->Reset();
                return CORE_E_INVALIDTYPE;
            }
        }

        const WCHAR* AsStringBufferAndCount(_Out_ XUINT32* pCount) const
        {
            if (valueString == m_type)
            {
                return m_estrValue.GetBufferAndCount(pCount);
            }
            else
            {
                *pCount = 0;
                return nullptr;
            }
        }

        _Check_return_ HRESULT GetTimeSpan(_Out_ wf::TimeSpan& pValue) const
        {
            pValue.Duration = m_lValue;
            return m_type == valueTimeSpan ? S_OK : CORE_E_INVALIDTYPE;
        }

        wf::TimeSpan AsTimeSpan() const
        {
            wf::TimeSpan result;

            if (m_type == valueTimeSpan)
            {
                result.Duration = m_lValue;
            }

            return result;
        }
        _Check_return_ HRESULT GetDateTime(_Out_ wf::DateTime& pValue) const
        {
            pValue.UniversalTime = m_lValue;
            return m_type == valueDateTime ? S_OK : CORE_E_INVALIDTYPE;
        }

        XPOINTF* AsPoint() const
        {
            return m_type == valuePoint ? reinterpret_cast<XPOINTF*>(m_peValue) : nullptr;
        }
        _Check_return_ HRESULT GetPoint(XPOINTF*& val) const
        {
            val = AsPoint();
            return m_type == valuePoint ? S_OK : CORE_E_INVALIDTYPE;
        }

        XRECTF* AsRect() const
        {
            return m_type == valueRect ? reinterpret_cast<XRECTF*>(m_peValue) : nullptr;
        }
        _Check_return_ HRESULT GetRect(XRECTF*& val) const
        {
            val = AsRect();
            return m_type == valueRect ? S_OK : CORE_E_INVALIDTYPE;
        }

        CDependencyObject* AsObject() const
        {
            // Note: You should probably use do_pointer_cast<T>() to get a DependencyObject of the proper type.
            // A type of valueNull correctly returns nullptr.
            return m_type == valueObject ? m_pdoValue : nullptr;
        }

        _Check_return_ HRESULT GetObject(CDependencyObject*& val) const
        {
            // Note: You should probably use DoPointerCast<T>() to get a DependencyObject of the proper type.
            val = AsObject();
            return (m_type == valueObject || m_type == valueNull) ? S_OK : CORE_E_INVALIDTYPE;
        }

        void* AsPointer() const
        {
            return m_type == valuePointer ? m_pvValue : nullptr;
        }

        _Check_return_ HRESULT GetPointer(void*& val) const
        {
            val = AsPointer();
            return (m_type == valuePointer || m_type == valueNull) ? S_OK : CORE_E_INVALIDTYPE;
        }
        ctl::ComPtr<IInspectable> DetachIInspectable();
        ctl::ComPtr<IUnknown> DetachIUnknown();
        xref_ptr<CDependencyObject> DetachObject();

        IUnknown* AsIUnknown() const
        {
            return m_type == valueIUnknown ? m_pUnkValue : nullptr;
        }

        IInspectable* AsIInspectable() const
        {
            return m_type == valueIInspectable ? m_pInspValue : nullptr;
        }

        XTHICKNESS* AsThickness() const
        {
            return m_type == valueThickness ? reinterpret_cast<XTHICKNESS*>(m_peValue) : nullptr;
        }
        _Check_return_ HRESULT GetThickness(XTHICKNESS*& val) const
        {
            val = AsThickness();
            return m_type == valueThickness ? S_OK : CORE_E_INVALIDTYPE;
        }

        INTERNAL_EVENT_HANDLER AsInternalHandler() const
        {
            return m_type == valueInternalHandler ? m_pHandler : nullptr;
        }
        _Check_return_ HRESULT GetInternalHandler(INTERNAL_EVENT_HANDLER& val) const
        {
            val = AsInternalHandler();
            return m_type == valueInternalHandler ? S_OK : CORE_E_INVALIDTYPE;
        }

        XSIZEF* AsSize() const
        {
            return m_type == valueSize ? reinterpret_cast<XSIZEF*>(m_peValue) : nullptr;
        }

        _Check_return_ HRESULT GetSize(XSIZEF*& val) const
        {
            val = AsSize();
            return m_type == valueSize ? S_OK : CORE_E_INVALIDTYPE;
        }

        XGRIDLENGTH* AsGridLength() const
        {
            return m_type == valueGridLength ? reinterpret_cast<XGRIDLENGTH*>(m_peValue) : nullptr;
        }

        _Check_return_ HRESULT GetGridLength(XGRIDLENGTH*& val) const
        {
            val = AsGridLength();
            return m_type == valueGridLength ? S_OK : CORE_E_INVALIDTYPE;
        }

        XFLOAT* AsFloatArray() const
        {
            return m_type == valueFloatArray ? m_peValue : nullptr;
        }

        _Check_return_ HRESULT GetFloatArray(XFLOAT*& val, unsigned* length) const
        {
            *length = GetArrayElementCount();
            val = AsFloatArray();
            return m_type == valueFloatArray ? S_OK : CORE_E_INVALIDTYPE;
        }

        XINT32* AsSignedArray() const
        {
            return m_type == valueSignedArray ? m_piValue : nullptr;
        }

        _Check_return_ HRESULT GetSignedArray(XINT32*& val, unsigned* length) const
        {
            *length = GetArrayElementCount();
            val = AsSignedArray();
            return m_type == valueSignedArray ? S_OK : CORE_E_INVALIDTYPE;
        }

        XDOUBLE* AsDoubleArray() const
        {
            return m_type == valueDoubleArray ? m_pdfValue : nullptr;
        }

        _Check_return_ HRESULT GetDoubleArray(XDOUBLE*& val, unsigned* length) const
        {
            *length = GetArrayElementCount();
            val = AsDoubleArray();
            return m_type == valueDoubleArray ? S_OK : CORE_E_INVALIDTYPE;
        }

        XCORNERRADIUS* AsCornerRadius() const
        {
            return m_type == valueCornerRadius ? reinterpret_cast<XCORNERRADIUS*>(m_peValue) : nullptr;
        }

        _Check_return_ HRESULT GetCornerRadius(XCORNERRADIUS*& val) const
        {
            val = AsCornerRadius();
            return m_type == valueCornerRadius ? S_OK : CORE_E_INVALIDTYPE;
        }

        bool IsNull() const
        {
            return m_type == valueNull ||
                (m_type == valueTypeHandle && m_tValue == KnownTypeIndex::UnknownType) ||
                (m_type == valueObject && m_pdoValue == nullptr) ||
                (m_type == valueString && m_estrValue.IsNull()) ||
                (m_type == valueInternalHandler && m_pHandler == nullptr) ||
                (m_type == valueIUnknown  && m_pvValue == nullptr) ||
                (m_type == valuePointer  && m_pvValue == nullptr);
        }

        bool IsUnset() const
        {
            return valueAny == m_type;
        }

        bool IsNullOrUnset() const
        {
            return IsUnset()
                || IsNull();
        }

        ValueType GetType() const
        {
            return m_type;
        }

        XUINT32 GetArrayElementCount() const
        {
            ASSERT(valueString != m_type);
            return m_internalCount;
        }

        void SetArrayElementCount(XUINT32 count)
        {
            ASSERT(valueString != m_type);
            m_internalCount = count;
        }

        bool OwnsValue() const
        {
            return m_fOwnsValue;
        }

        XPOINTF* AsPointArray() const
        {
            return m_type == valuePointArray ? reinterpret_cast<XPOINTF*>(m_peValue) : nullptr;
        }

        _Check_return_ HRESULT GetPointArray(XPOINTF*& val, unsigned* length) const
        {
            *length = GetArrayElementCount();
            val = AsPointArray();
            return m_type == valuePointArray ? S_OK : CORE_E_INVALIDTYPE;
        }

    private:
        // copy assignment operator
        CValue& operator=(const CValue &other);

        template<typename TSource, typename TDest>
        static _Check_return_ HRESULT ConvertForManaged(
            _Out_ TDest *pData,
            _In_ const TSource& value);

        void ResetValue(
            ValueType type,
            XUINT32 count,
            bool fOwnsValue = true)
        {
            ReleaseAndReset();

            m_internalCount = count;
            m_type = type;
            m_fOwnsValue = fOwnsValue;
        }

        friend class ::CDependencyObject;

    public:
        union
        {
            struct
            {
                XUINT32     m_internalCount : 24;   // Array limit is 16777215 elements
                // When holding a WCHAR string, this is the count of *characters*,
                //  following strlen() convention which does NOT count the null terminator.
                ValueType   m_type : 6;    // Current type of this value
                XUINT32     m_fOwnsValue : 1;
                XUINT32     m_isIndependent : 1;    // True iff this DP value can also change independently on the render thread
            };
            XUINT32 CountTypeAndFlags;
        };

        // Hide the implementation details from external callers
        XUINT32 EncodeCountTypeAndFlags() const { return CountTypeAndFlags; }
        void ResetCountTypeAndFlags() { CountTypeAndFlags = 0; }

        union
        {
            void               *m_pvValue;  // Generic pointer value
            XFLOAT              m_eValue;   // 32 bit real value
            XINT32              m_iValue;   // 32 bit signed integer
            XINT64              m_lValue;   // 64 bit signed integer
            KnownTypeIndex      m_tValue;
            DOUBLE              m_dValue;   // 64 bit double
            XUINT32             m_nValue;   // Boolean or color value
            CValueEnum          m_enumValue; // Enum value
            xencoded_string_ptr m_estrValue; // Encoded string value
            XFLOAT             *m_peValue;  // Array value
            XDOUBLE            *m_pdfValue;  // Array value
            XUINT8             *m_peByteValue; //Array of Bytes
            CDependencyObject  *m_pdoValue; // Object value
            IUnknown           *m_pUnkValue; // IUnknown value
            IInspectable       *m_pInspValue; // IInspectable value
            XINT32             *m_piValue;   // Array of 32 bit signed
            INTERNAL_EVENT_HANDLER m_pHandler; // Method pointer to internal handler
            CThemeResource* m_pThemeResource;
        };
    };

}
