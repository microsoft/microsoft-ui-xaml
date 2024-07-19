// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ComObject.h>
#include <InterfaceForwarder.h>

#ifndef EXP_CLANG
    // MSVC happily accepts static template specializations.
    #define STATIC_SPEC static
#else
    // Clang does not and emits a warning.
    #define STATIC_SPEC
#endif

namespace DirectUI
{
    enum class MarkupExtensionType : uint8_t;
    enum class PrintDocumentFormat : uint8_t;
    enum class TextFormattingMode : uint8_t;
    enum class TextHintingMode : uint8_t;
    enum class TextRenderingMode : uint8_t;
    enum class GestureModes : uint8_t;
    enum class PointerDirection : uint8_t;
}

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft::UI::Xaml::Data
{
    struct TextRange;
}
XAML_ABI_NAMESPACE_END

enum class KnownTypeIndex : uint16_t;

XAML_ABI_NAMESPACE_BEGIN namespace Windows
{
    namespace Foundation
    {
        // These aren't actually used anywhere. They're just necessary for the time being to make DO::SetValueByKnownIndex/GetValueByKnownIndex to compile.
        // Eventually we should remove this code, and update SetValueByKnownIndex/GetValueByKnownIndex to properly deal with internal enums in such a way
        // that it doesn't require definitions for IReference<T>.
        template <> struct __declspec(uuid("c328a07d-590e-4ca9-9e8e-1167e93c0988")) wf::IReference<DirectUI::MarkupExtensionType> : IReference_impl<DirectUI::MarkupExtensionType>{};
        template <> struct __declspec(uuid("993b3b30-59a9-4944-8802-35f6c2ebb11d")) wf::IReference<DirectUI::PrintDocumentFormat> : IReference_impl<DirectUI::PrintDocumentFormat>{};
        template <> struct __declspec(uuid("67a17862-e4ff-4ed5-82d4-8d7fd5705f4d")) wf::IReference<DirectUI::TextFormattingMode> : IReference_impl<DirectUI::TextFormattingMode>{};
        template <> struct __declspec(uuid("693ca33f-29f1-4b8f-be6f-ffa15d50f243")) wf::IReference<DirectUI::TextHintingMode> : IReference_impl<DirectUI::TextHintingMode>{};
        template <> struct __declspec(uuid("debcb27f-9355-4181-a6ea-a08aecc62b25")) wf::IReference<DirectUI::TextRenderingMode> : IReference_impl<DirectUI::TextRenderingMode>{};
        template <> struct __declspec(uuid("d098eaf6-6b3f-4047-9273-137f9546fdb7")) wf::IReference<DirectUI::GestureModes> : IReference_impl<DirectUI::GestureModes>{};
        template <> struct __declspec(uuid("d5893666-412f-450f-ad82-d2b482d50178")) wf::IReference<DirectUI::PointerDirection> : IReference_impl<DirectUI::PointerDirection>{};
    }
} XAML_ABI_NAMESPACE_END

#pragma region forwarders
namespace ctl
{
    template<typename impl_type, typename T>
    class interface_forwarder<wf::IReference<T>, impl_type> final
        : public ctl::iinspectable_forwarder_base<wf::IReference<T>, impl_type>
    {
        impl_type* This() { return this->template This_helper<impl_type>(); }
        IFACEMETHODIMP get_Value(_Out_ T* pValue) override { return This()->get_Value(pValue); }
    };

    template<typename impl_type>
    class interface_forwarder<wf::IPropertyValue, impl_type> final
        : public ctl::iinspectable_forwarder_base<wf::IPropertyValue, impl_type>
    {
        impl_type* This() { return this->template This_helper<impl_type>(); }
        IFACEMETHODIMP get_Type(_Out_ wf::PropertyType* valueType) override { return This()->get_Type(valueType); }
        IFACEMETHODIMP get_IsNumericScalar(_Out_ BOOLEAN* value) override { return This()->get_IsNumericScalar(value); }
        IFACEMETHODIMP GetUInt8(_Out_ BYTE *value) override { return This()->GetUInt8(value); }
        IFACEMETHODIMP GetUInt16(_Out_ UINT16 *value) override { return This()->GetUInt16(value); }
        IFACEMETHODIMP GetInt16(_Out_ INT16 *value) override { return This()->GetInt16(value); }
        IFACEMETHODIMP GetInt32(_Out_ INT32 *value) override { return This()->GetInt32(value); }
        IFACEMETHODIMP GetUInt32(_Out_ UINT32 *value) override { return This()->GetUInt32(value); }
        IFACEMETHODIMP GetInt64(_Out_ INT64 *value) override { return This()->GetInt64(value); }
        IFACEMETHODIMP GetUInt64(_Out_ UINT64 *value) override { return This()->GetUInt64(value); }
        IFACEMETHODIMP GetSingle(_Out_ FLOAT *value) override { return This()->GetSingle(value); }
        IFACEMETHODIMP GetDouble(_Out_ DOUBLE *value) override { return This()->GetDouble(value); }
        IFACEMETHODIMP GetChar16(_Out_ WCHAR *value) override { return This()->GetChar16(value); }
        IFACEMETHODIMP GetBoolean(_Out_ BOOLEAN *value) override { return This()->GetBoolean(value); }
        IFACEMETHODIMP GetString(_Outptr_result_maybenull_ HSTRING *value) override { return This()->GetString(value); }
        IFACEMETHODIMP GetGuid(_Out_ GUID *value) override { return This()->GetGuid(value); }
        IFACEMETHODIMP GetDateTime(_Out_ wf::DateTime *value) override { return This()->GetDateTime(value); }
        IFACEMETHODIMP GetTimeSpan(_Out_ wf::TimeSpan *value) override { return This()->GetTimeSpan(value); }
        IFACEMETHODIMP GetPoint(_Out_ wf::Point *value) override { return This()->GetPoint(value); }
        IFACEMETHODIMP GetSize(_Out_ wf::Size *value) override { return This()->GetSize(value); }
        IFACEMETHODIMP GetRect(_Out_ wf::Rect *value) override { return This()->GetRect(value); }
        IFACEMETHODIMP GetUInt8Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) BYTE **value) override { return This()->GetUInt8Array(length, value); }
        IFACEMETHODIMP GetUInt16Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) UINT16 **value) override { return This()->GetUInt16Array(length, value); }
        IFACEMETHODIMP GetInt16Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) INT16 **value) override { return This()->GetInt16Array(length, value); }
        IFACEMETHODIMP GetInt32Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) INT32 **value) override { return This()->GetInt32Array(length, value); }
        IFACEMETHODIMP GetUInt32Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) UINT32 **value) override { return This()->GetUInt32Array(length, value); }
        IFACEMETHODIMP GetInt64Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) INT64 **value) override { return This()->GetInt64Array(length, value); }
        IFACEMETHODIMP GetUInt64Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) UINT64 **value) override { return This()->GetUInt64Array(length, value); }
        IFACEMETHODIMP GetSingleArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) FLOAT **value) override { return This()->GetSingleArray(length, value); }
        IFACEMETHODIMP GetDoubleArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) DOUBLE **value) override { return This()->GetDoubleArray(length, value); }
        IFACEMETHODIMP GetChar16Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) WCHAR **value) override { return This()->GetChar16Array(length, value); }
        IFACEMETHODIMP GetBooleanArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) BOOLEAN **value) override { return This()->GetBooleanArray(length, value); }
        IFACEMETHODIMP GetStringArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) HSTRING **value) override { return This()->GetStringArray(length, value); }
        IFACEMETHODIMP GetInspectableArray(_Out_ UINT32 *length, _Out_ IInspectable ***value) override { return This()->GetInspectableArray(length, value); }
        IFACEMETHODIMP GetGuidArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) GUID **value) override { return This()->GetGuidArray(length, value); }
        IFACEMETHODIMP GetDateTimeArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::DateTime **value) override { return This()->GetDateTimeArray(length, value); }
        IFACEMETHODIMP GetTimeSpanArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::TimeSpan **value) override { return This()->GetTimeSpanArray(length, value); }
        IFACEMETHODIMP GetPointArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::Point **value) override { return This()->GetPointArray(length, value); }
        IFACEMETHODIMP GetSizeArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::Size **value) override { return This()->GetSizeArray(length, value); }
        IFACEMETHODIMP GetRectArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::Rect **value) override { return This()->GetRectArray(length, value); }
    };
}

namespace DirectUI
{
    class PropertyValue;

    namespace ReferenceDetails
    {
        template<typename T>
        struct ReferenceTraits
        {
            using value_type = T;
            static void Destroy(value_type&) { }
            static STDMETHODIMP Set(value_type& member, const value_type& param) { member = param; return S_OK; }
            static STDMETHODIMP Get(const value_type& member, value_type* param) { *param = member; return S_OK; }
        };

        template<>
        struct ReferenceTraits<wxaml_interop::TypeName>
        {
            using value_type = wxaml_interop::TypeName;
            static void Destroy(value_type& member);
            static STDMETHODIMP Set(value_type& member, const value_type& param);
            static STDMETHODIMP Get(const value_type& member, value_type* param);
        };
    }

    template<class T>
    class __declspec(novtable) ReferenceBase
        : public ctl::ComBase
    {
        friend class PropertyValue;
        using Traits = ReferenceDetails::ReferenceTraits<T>;

    protected:
        ReferenceBase() : m_value() {}

    public:
        ~ReferenceBase() override
        {
            Traits::Destroy(m_value);
        }

        STDMETHODIMP get_Value(_Out_ T* pValue)
        {
            IFCPTR_RETURN(pValue);
            IFC_RETURN(Traits::Get(m_value, pValue));
            return S_OK;
        }

        STDMETHODIMP get_Type(_Out_ wf::PropertyType *valueType)
        {
            IFCPTR_RETURN(valueType);
            *valueType = wf::PropertyType_OtherType;
            return S_OK;
        }

        STDMETHODIMP GetUInt8(_Out_ BYTE *value) { return E_NOTIMPL; }
        STDMETHODIMP GetUInt16(_Out_ UINT16 *value) { return E_NOTIMPL; }
        STDMETHODIMP GetInt16(_Out_ INT16 *value) { return E_NOTIMPL; }
        STDMETHODIMP GetInt32(_Out_ INT32 *value) { return E_NOTIMPL; }
        STDMETHODIMP GetUInt32(_Out_ UINT32 *value) { return E_NOTIMPL; }
        STDMETHODIMP GetInt64(_Out_ INT64 *value) { return E_NOTIMPL; }
        STDMETHODIMP GetUInt64(_Out_ UINT64 *value) { return E_NOTIMPL; }
        STDMETHODIMP GetSingle(_Out_ FLOAT *value) { return E_NOTIMPL; }
        STDMETHODIMP GetDouble(_Out_ DOUBLE *value) { return E_NOTIMPL; }
        STDMETHODIMP GetChar16(_Out_ WCHAR *value) { return E_NOTIMPL; }
        STDMETHODIMP GetBoolean(_Out_ BOOLEAN *value) { return E_NOTIMPL; }
        STDMETHODIMP GetString(_Outptr_result_maybenull_ HSTRING *value) { return E_NOTIMPL; }
        STDMETHODIMP GetGuid(_Out_ GUID *value) { return E_NOTIMPL; }
        STDMETHODIMP GetDateTime(_Out_ wf::DateTime *value) { return E_NOTIMPL; }
        STDMETHODIMP GetTimeSpan(_Out_ wf::TimeSpan *value) { return E_NOTIMPL; }
        STDMETHODIMP GetPoint(_Out_ wf::Point *value) { return E_NOTIMPL; }
        STDMETHODIMP GetSize(_Out_ wf::Size *value) { return E_NOTIMPL; }
        STDMETHODIMP GetRect(_Out_ wf::Rect *value) { return E_NOTIMPL; }
        STDMETHODIMP GetUInt8Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) BYTE **value) { return E_NOTIMPL; }
        STDMETHODIMP GetUInt16Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) UINT16 **value) { return E_NOTIMPL; }
        STDMETHODIMP GetInt16Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) INT16 **value) { return E_NOTIMPL; }
        STDMETHODIMP GetInt32Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) INT32 **value) { return E_NOTIMPL; }
        STDMETHODIMP GetUInt32Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) UINT32 **value) { return E_NOTIMPL; }
        STDMETHODIMP GetInt64Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) INT64 **value) { return E_NOTIMPL; }
        STDMETHODIMP GetUInt64Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) UINT64 **value) { return E_NOTIMPL; }
        STDMETHODIMP GetSingleArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) FLOAT **value) { return E_NOTIMPL; }
        STDMETHODIMP GetDoubleArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) DOUBLE **value) { return E_NOTIMPL; }
        STDMETHODIMP GetChar16Array(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) WCHAR **value) { return E_NOTIMPL; }
        STDMETHODIMP GetBooleanArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) BOOLEAN **value) { return E_NOTIMPL; }
        STDMETHODIMP GetStringArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) HSTRING **value) { return E_NOTIMPL; }
        STDMETHODIMP GetInspectableArray(_Out_ UINT32 *length, _Out_ IInspectable ***value) { return E_NOTIMPL; }
        STDMETHODIMP GetGuidArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) GUID **value) { return E_NOTIMPL; }
        STDMETHODIMP GetDateTimeArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::DateTime **value) { return E_NOTIMPL; }
        STDMETHODIMP GetTimeSpanArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::TimeSpan **value) { return E_NOTIMPL; }
        STDMETHODIMP GetPointArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::Point **value) { return E_NOTIMPL; }
        STDMETHODIMP GetSizeArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::Size **value) { return E_NOTIMPL; }
        STDMETHODIMP GetRectArray(_Out_ UINT32 *length, _Outptr_result_buffer_all_maybenull_(*length) wf::Rect **value) { return E_NOTIMPL; }

    protected:
        _Check_return_ HRESULT GetRuntimeClassNameImpl(_Out_ HSTRING* pClassName) final;

        _Check_return_ HRESULT SetValue(T value)
        {
            return Traits::Set(m_value, value);
        }

        T m_value;
    };

    template<class T>
    class __declspec(novtable) Reference:
        public ReferenceBase<T>,
        public ctl::forwarder_holder<wf::IPropertyValue, Reference<T>>,
        public ctl::forwarder_holder<wf::IReference<T>, Reference<T>>
    {
        friend class PropertyValue;
        using BaseType = ReferenceBase<T>;

        BEGIN_INTERFACE_MAP(Reference, ReferenceBase<T>)
            INTERFACE_ENTRY(Reference, wf::IPropertyValue)
            INTERFACE_ENTRY(Reference, wf::IReference<T>)
        END_INTERFACE_MAP(Reference, ReferenceBase<T>)

    public:
        Reference() {}
        STDMETHODIMP get_IsNumericScalar(_Out_ BOOLEAN *value)
        {
            *value = FALSE;
            return S_OK;
        }

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wf::IPropertyValue)))
            {
                *ppObject = ctl::interface_cast<wf::IPropertyValue>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wf::IReference<T>)))
            {
                *ppObject = ctl::interface_cast<wf::IReference<T>>(this);
            }
            else
            {
                return ReferenceBase<T>::QueryInterfaceImpl(iid, ppObject);
            }

            this->AddRefOuter();
            return S_OK;
        }
    };

    template<class T>
    class __declspec(novtable) EnumReference
        : public ReferenceBase<T>
        , public ctl::forwarder_holder<wf::IPropertyValue, EnumReference<T>>
        , public ctl::forwarder_holder<wf::IReference<T>, EnumReference<T>>
        , public ctl::forwarder_holder<wf::IReference<INT>, EnumReference<T>>
    {
        // Strangely, the original implementation of EnumReference never added IReference<INT> to the
        // implemented interfaces list. Not sure why that is. Unlikely to hurt anything, though, and
        // who knows what will break if this changes. It's not easily quirkable...
        BEGIN_INTERFACE_MAP(EnumReference, ReferenceBase<T>)
            INTERFACE_ENTRY(EnumReference, wf::IPropertyValue)
            INTERFACE_ENTRY(EnumReference, wf::IReference<T>)
        END_INTERFACE_MAP(EnumReference, ReferenceBase<T>)
    public:

        using ReferenceBase<T>::get_Value;
        STDMETHODIMP get_Value(_Out_ INT* pValue)
        {
            return GetInt32(pValue);
        }

        STDMETHODIMP get_IsNumericScalar(_Out_ BOOLEAN *value)
        {
            *value = TRUE;
            RRETURN(S_OK);
        }

        STDMETHODIMP GetInt32(_Out_ INT32 *value)
        {
            *value = static_cast<INT32>(this->m_value);
            RRETURN(S_OK);
        }

        STDMETHODIMP GetUInt32(_Out_ UINT32 *value)
        {
            *value = static_cast<UINT32>(this->m_value);
            RRETURN(S_OK);
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        EnumReference() = default;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wf::IReference<INT>)))
            {
                *ppObject = ctl::interface_cast<wf::IReference<INT>>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wf::IPropertyValue)))
            {
                *ppObject = ctl::interface_cast<wf::IPropertyValue>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wf::IReference<T>)))
            {
                *ppObject = ctl::interface_cast<wf::IReference<T>>(this);
            }
            else
            {
                return ReferenceBase<T>::QueryInterfaceImpl(iid, ppObject);
            }

            this->AddRefOuter();
            return S_OK;
        }
    };

    class PropertyValue
    {
    public:

        static _Check_return_ HRESULT AreEqual(_In_ IInspectable* oldValue, _In_ IInspectable* newValue, _Out_ bool* areEqual) noexcept;

        __inline static bool IsNumericPropertyType(_In_ wf::PropertyType valueType)
        {
            return valueType == wf::PropertyType_UInt8 ||
                valueType == wf::PropertyType_Char16   ||
                valueType == wf::PropertyType_Int16    ||
                valueType == wf::PropertyType_UInt16   ||
                valueType == wf::PropertyType_Int32    ||
                valueType == wf::PropertyType_UInt32   ||
                valueType == wf::PropertyType_Int64    ||
                valueType == wf::PropertyType_UInt64   ||
                valueType == wf::PropertyType_Single   ||
                valueType == wf::PropertyType_Double;
        }

        template<class T>
        static _Check_return_ HRESULT CreateReference(_In_ T value, _Outptr_ IInspectable **ppValue)
        {
            ctl::ComPtr<wf::IReference<T>> ref;
            IFC_RETURN(CreateTypedReference(value, ref.ReleaseAndGetAddressOf()));

            *ppValue = ctl::as_iinspectable(ref.Detach());
            return S_OK;
        }

        template<class T>
        static _Check_return_ HRESULT CreateTypedReference(_In_ T value, _Outptr_ wf::IReference<T> **ppValue)
        {
            IFCPTR_RETURN(ppValue);

            ctl::ComPtr<Reference<T>> ref;
            IFC_RETURN(ctl::ComObject<Reference<T>>::CreateInstance(&ref));
            IFC_RETURN(ref->SetValue(value));

            ctl::ComPtr<wf::IReference<T>> refAsI;
            IFC_RETURN(ref.As(&refAsI));

            *ppValue = refAsI.Detach();
            return S_OK;
        }

        template<class T>
        static _Check_return_ HRESULT CreateEnumReference(_In_ T value, _Outptr_ IInspectable **ppValue)
        {
            ctl::ComPtr<EnumReference<T>> spRef;

            IFCPTR_RETURN(ppValue);
            IFC_RETURN(ctl::ComObject<EnumReference<T>>::CreateInstance(&spRef));
            IFC_RETURN(spRef->SetValue(value));

            *ppValue = ctl::as_iinspectable(spRef.Detach());

            return S_OK;
        }

        static _Check_return_ HRESULT CreateEmpty(_Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromBoolean(_In_ BOOLEAN bValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromUInt8(_In_ BYTE nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromInt16(_In_ INT16 nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromUInt16(_In_ UINT16 nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromInt32(_In_ INT32 nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromUInt32(_In_ UINT32 nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromInt64(_In_ INT64 nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromUInt64(_In_ UINT64 nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromDouble(_In_ XDOUBLE nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromSingle(_In_ XFLOAT nValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromString(_In_opt_ HSTRING hString, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromChar16(_In_ WCHAR chValue, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromTimeSpan(_In_ wf::TimeSpan value, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromDateTime(_In_ wf::DateTime value, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromPoint(_In_ wf::Point value, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromSize(_In_ wf::Size value, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromRect(_In_ wf::Rect value, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromGuid(_In_ GUID value, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromVisibility(_In_ xaml::Visibility visibility, _Outptr_ IInspectable **ppValue);
        static _Check_return_ HRESULT CreateFromTextRange(_In_ xaml_docs::TextRange value, _Outptr_ IInspectable **ppValue);

        static bool IsNullOrEmpty(_In_ IInspectable *pValue);

        // We provide template specilizations for primitive types since CreateReference will by default
        // create a PropertyValue with type OtherType. This breaks debug tools that need to understand the type.
        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ bool value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromBoolean(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ BYTE value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromUInt8(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ int16_t value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromInt16(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ uint16_t value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromUInt16(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ int32_t value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromInt32(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ uint32_t value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromUInt32(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ int64_t value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromInt64(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ uint64_t value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromUInt64(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ double value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromDouble(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ float value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromSingle(value, ppValue);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT CreateReference(_In_ HSTRING value, _Outptr_ IInspectable **ppValue)
        {
            return CreateFromString(value, ppValue);
        }
    };

    MIDL_INTERFACE("a7a1f8c9-ddd1-4541-ac57-3f0dc02f4b89")
    IWeakInspectable : public IInspectable
    {
        virtual _Check_return_ HRESULT GetInspectable(_Outptr_ IInspectable **ppValue) = 0;
    };

    class ValueWeakReference: public ctl::implements_inspectable<IWeakInspectable>
    {
    private:

        ValueWeakReference(): m_pWeakReference(NULL)
        { }

        ~ValueWeakReference() override
        {
            ReleaseInterface(m_pWeakReference);
        }

    public:

        _Check_return_ HRESULT GetInspectable(_Outptr_ IInspectable **value) override;

    public:

        static _Check_return_ HRESULT Create(_In_ IInspectable *pObject, _Outptr_ IInspectable **ppValue);

        template <class T>
        static T* get_value_as(_In_ IInspectable *pSource)
        {
            HRESULT hr = S_OK;
            IWeakInspectable *pWeakRef = NULL;
            IInspectable *pInsp = NULL;
            T *pResult = NULL;

            pWeakRef = ctl::query_interface<IWeakInspectable>(pSource);
            if (pWeakRef)
            {
                IFC(pWeakRef->GetInspectable(&pInsp));
            }
            else
            {
                pInsp = pSource;
                AddRefInterface(pInsp);
            }

            pResult = ctl::query_interface<T>(pInsp);

        Cleanup:

            ReleaseInterface(pWeakRef);
            ReleaseInterface(pInsp);

            return pResult;
        }

    private:

        IWeakReference *m_pWeakReference;
    };
}
