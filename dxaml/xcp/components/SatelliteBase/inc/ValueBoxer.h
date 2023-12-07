// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlWrlHelpers.h>
#include <type_traits>

namespace Private
{

// Helper for properly implementing IPropertyValue.GetInt32 and IPropertyValue.GetUInt32 for enum references.
struct ReferenceHelpers
{
    template<typename T>
    static _Check_return_ typename std::enable_if<std::is_enum<T>::value, HRESULT>::type GetInt32(T& valRef, _Out_ INT32 * value )
    {
        *value = static_cast<INT32>(valRef);
        return S_OK;
    }

    template<typename T>
    static _Check_return_ typename std::enable_if<std::is_enum<T>::value, HRESULT>::type GetUInt32(T& valRef, _Out_ UINT32 * value)
    {
        *value = static_cast<UINT32>(valRef);
        return S_OK;
    }

    template<typename T>
    static _Check_return_ typename std::enable_if<!std::is_enum<T>::value, HRESULT>::type GetInt32(T&, _Out_ INT32 *) { return E_NOTIMPL; }

    template<typename T>
    static _Check_return_ typename std::enable_if<!std::is_enum<T>::value, HRESULT>::type GetUInt32(T&, _Out_ UINT32 *) { return E_NOTIMPL; }
};

// Helper class to allow boxing of enums and structs. Implementing
// IReference requires that we also implement IPropertyValue, as they
// 'inherit' from each other according to MSDN.
template <typename T>
class Reference :
    public Microsoft::WRL::RuntimeClass<
        wf::IReference<T>,
        wf::IPropertyValue,
        Microsoft::WRL::FtmBase
        >
{
public:
    using RuntimeClassT = typename Microsoft::WRL::RuntimeClass<
        wf::IReference<T>,
        wf::IPropertyValue,
        Microsoft::WRL::FtmBase
        >::RuntimeClassT;

    InspectableClass(wf::IReference<T>::z_get_rc_name_impl(), TrustLevel::BaseTrust);

public:

    _Check_return_ HRESULT RuntimeClassInitialize(T val)
    {
        m_val = val;
        return S_OK;
    }

    // IReference Interface
    IFACEMETHODIMP get_Value(_Out_ T* pVal)
    {
        if (pVal == nullptr)
        {
            return E_INVALIDARG;
        }

        *pVal = m_val;
        return S_OK;
    }

    // IPropertyValue Interface
    STDMETHODIMP get_Type(_Out_ wf::PropertyType *valueType)  {
        // CValueBoxer needs enum references to be IPropertyValues and to
        // return OtherType as their type in order to call the correct EnumObject
        // boxing/unboxing functions.
        *valueType = wf::PropertyType::PropertyType_OtherType;
        return S_OK;
    }

    STDMETHODIMP get_IsNumericScalar(_Out_ boolean * /* value */) { return E_FAIL; }
    STDMETHODIMP GetInspectable(_Out_ IInspectable ** /* value */) { return E_FAIL; }
    STDMETHODIMP GetUInt8(_Out_ BYTE * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetUInt16(_Out_ UINT16 * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetInt16(_Out_ INT16 * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetInt32(_Out_ INT32 * value ) { return ReferenceHelpers::GetInt32(m_val, value); }
    STDMETHODIMP GetUInt32(_Out_ UINT32 * value ){ return ReferenceHelpers::GetUInt32(m_val, value); }
    STDMETHODIMP GetInt64(_Out_ INT64 * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetUInt64(_Out_ UINT64 * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetSingle(_Out_ FLOAT * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetDouble(_Out_ DOUBLE * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetChar16(_Out_ WCHAR * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetBoolean(_Out_ boolean * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetString(_Outptr_result_maybenull_ HSTRING * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetGuid(_Out_ GUID * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetDateTime(_Out_ wf::DateTime * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetTimeSpan(_Out_ wf::TimeSpan * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetPoint(_Out_ wf::Point * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetSize(_Out_ wf::Size * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetRect(_Out_ wf::Rect * /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetUInt8Array(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ BYTE ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetUInt16Array(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ UINT16 ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetInt16Array(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ INT16 ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetInt32Array(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ INT32 ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetUInt32Array(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ UINT32 ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetInt64Array(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ INT64 ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetUInt64Array(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ UINT64 ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetSingleArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ FLOAT ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetDoubleArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ DOUBLE ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetChar16Array(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ WCHAR ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetBooleanArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ boolean ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetStringArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ HSTRING ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetInspectableArray(_Out_ UINT32* /* length */, _Out_ IInspectable *** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetGuidArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ GUID ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetDateTimeArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ wf::DateTime ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetTimeSpanArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ wf::TimeSpan ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetPointArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ wf::Point ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetSizeArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ wf::Size ** /* value */) { return E_NOTIMPL; }
    STDMETHODIMP GetRectArray(_Out_ UINT32* /* length */, _Outptr_result_maybenull_ wf::Rect ** /* value */) { return E_NOTIMPL; }

private:
    T m_val;

}; // class Reference


class ValueBoxer
{
    template<class T>
        static _Check_return_ HRESULT CreatePropertyValue(_In_ T value, _Outptr_ IInspectable** ppPropertyValue)
        {
            // Specialize this templated method for every supported value type.
            // The default implementation assumes it's an enum or struct.
            // (see VALUEBOXER_METHOD macro).
            RRETURN(CreateReference<T>(value, ppPropertyValue));
        }

// First parameter is the value type to boxed.
// Second parameter is the name of the method in IPropertyValueStatics that will
// actually box the value.  It also becomes the method name in ValueBoxer.
#define VALUEBOXER_METHOD(type, boxingMethod) \
    static HRESULT boxingMethod(_In_ type value, _Outptr_ IInspectable** boxedValue) \
    { \
        HRESULT hr = E_INVALIDARG; \
            wrl::ComPtr<wf::IPropertyValueStatics> spStatics; \
            hr = wf::GetActivationFactory( \
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), \
                &spStatics); \
            if (SUCCEEDED(hr)) { hr = spStatics->boxingMethod(value, boxedValue); } \
        return hr; \
    } \
    template<> \
    static _Check_return_ HRESULT CreatePropertyValue(_In_ type value, _Outptr_ IInspectable** ppPropertyValue) \
        { \
            return boxingMethod(value, ppPropertyValue); \
        }

public:

    VALUEBOXER_METHOD(DOUBLE, CreateDouble);
    VALUEBOXER_METHOD(BOOLEAN, CreateBoolean);
    VALUEBOXER_METHOD(HSTRING, CreateString);
    VALUEBOXER_METHOD(INT32, CreateInt32);
    VALUEBOXER_METHOD(wf::DateTime, CreateDateTime);
    VALUEBOXER_METHOD(wf::TimeSpan, CreateTimeSpan);
    VALUEBOXER_METHOD(wf::Rect, CreateRect);
    VALUEBOXER_METHOD(wf::Size, CreateSize);
    VALUEBOXER_METHOD(wf::Point, CreatePoint);
    // Do not add more custom Create<MyType> here
    // Code generation should generate againts against the template CreateReference<T>
    // Add an specialized CreateReference<MyType> if the default implementation does not work for you.

    // Normally it will be simple for the property getter for a value-typed property to just
    // call Get<Type> on the IPropertyValue returned from DependencyObject->GetValue, but
    // HSTRING is a little tricky because it's allowed to be null despite being a value
    // type, so this helper is provided to standardize treatment of HSTRING typed properties.
    static _Check_return_ HRESULT UnboxString(_In_ IInspectable* propertyValue, _Out_ HSTRING* value)
    {
        wrl::ComPtr<wf::IPropertyValue> spPropertyValue;
        wrl::ComPtr<IInspectable> spPropertyValueAsI(propertyValue);

        IFCPTRRC_RETURN(value, E_INVALIDARG);

        if(!propertyValue)
        {
            ::WindowsCreateString(L"", 0, value);
        }
        else
        {
            wf::PropertyType propertyType = wf::PropertyType::PropertyType_Empty;
            IFC_RETURN(spPropertyValueAsI.As(&spPropertyValue));
            IFC_RETURN(spPropertyValue->get_Type(&propertyType));
            IFCEXPECT_RETURN(propertyType == wf::PropertyType::PropertyType_String);
            IFC_RETURN(spPropertyValue->GetString(value));
        }

        return S_OK;
    }

    template<typename T>
    static _Check_return_ HRESULT CreateReference(_In_ T value, _Outptr_ IInspectable **ppValue)
    {
        wrl::ComPtr<wf::IReference<T>> spReference;

        ASSERT(ppValue);

        IFC_RETURN(wrl::MakeAndInitialize<Reference<T>>(&spReference, value));

        *ppValue = iinspectable_cast(spReference.Detach());

        return S_OK;
    }

    template<class T>
    static _Check_return_ HRESULT EnumBoxer(_In_ UINT32 enumValue, _Outptr_ IInspectable **ppBoxedEnum)
    {
        return CreateReference<T>(static_cast<T>(enumValue), ppBoxedEnum);
    }
};

}
