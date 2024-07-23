// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CValue.h>
#include <CValueUtil.h>
#include <TypeTableStructs.h> // uses CDependencyProperty
#include <Value.h> // uses PropertyValue

#ifndef EXP_CLANG
    // MSVC happily accepts static template specializations.
    #define STATIC_SPEC static
#else
    // Clang does not and emits a warning.
    #define STATIC_SPEC
#endif

class CClassInfo;
class CDependencyObject;

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft::UI::Xaml::Data
{
    struct TextRange;
}
XAML_ABI_NAMESPACE_END

namespace DirectUI
{
    class DependencyObject;
    class BoxerBuffer;

    template <typename T>
    struct NeedsBoxerBuffer
    {
        static const bool value = false;
    };

    template <>
    struct NeedsBoxerBuffer<wf::Point>
    {
        static const bool value = true;
    };

    template <>
    struct NeedsBoxerBuffer<wf::Size>
    {
        static const bool value = true;
    };

    template <>
    struct NeedsBoxerBuffer<wf::Rect>
    {
        static const bool value = true;
    };

    template <>
    struct NeedsBoxerBuffer<xaml::CornerRadius>
    {
        static const bool value = true;
    };

    template <>
    struct NeedsBoxerBuffer<xaml::Thickness>
    {
        static const bool value = true;
    };

    template <>
    struct NeedsBoxerBuffer<xaml::GridLength>
    {
        static const bool value = true;
    };

    template <>
    struct NeedsBoxerBuffer<xaml_media::Matrix>
    {
        static const bool value = true;
    };

    template <>
    struct NeedsBoxerBuffer<xaml_media::Media3D::Matrix3D>
    {
        static const bool value = true;
    };

    class CValueBoxer
    {
    public:
        CValueBoxer() = delete;

        // Default behavior for copying values.
        template<class T>
        static _Check_return_ HRESULT CopyValue(
            _In_ const T& value,
            _Out_ T* pCopy)
        {
            *pCopy = value;
            return S_OK;
        }

        // TypeNames need to duplicate the Name string.
        static _Check_return_ HRESULT CopyValue(
            _In_ const wxaml_interop::TypeName& value,
            _Out_ wxaml_interop::TypeName* pCopy)
        {
            pCopy->Kind = value.Kind;
            return WindowsDuplicateString(value.Name, &pCopy->Name);
        }

        // Temporary catch-all.
        template<class T, class U>
        static _Check_return_ typename std::enable_if<!std::is_enum<T>::value, HRESULT>::type ConvertToFramework(
            _In_ const T& coreValue,
            _Outptr_ U* pFrameworkValue,
            _In_ BOOLEAN fReleaseCoreValue)
        {
            return E_NOTIMPL;
        }

        // Convert from internal enums (e.g. DirectUI::VirtualKey) to public enums (e.g. wsy::VirtualKey).
        template<class T, class U>
        static _Check_return_ typename std::enable_if<std::is_enum<T>::value, HRESULT>::type ConvertToFramework(
            _In_ const T& coreValue,
            _Out_ U* pFrameworkValue,
            _In_ BOOLEAN fReleaseCoreValue)
        {
            *pFrameworkValue = static_cast<U>(coreValue);
            return S_OK;
        }

        template<class T>
        static _Check_return_ HRESULT ConvertToFramework(
            _In_ const T& coreValue,
            _Out_ T* pFrameworkValue,
            _In_ BOOLEAN fReleaseCoreValue)
        {
            *pFrameworkValue = coreValue;
            return S_OK;
        }

        // Conversion from a core DO to a framework DO.
        template<class T, class U>
        static _Check_return_ HRESULT ConvertToFramework(
            _In_opt_ T* pCoreObject,
            _Outptr_result_maybenull_ U** ppFrameworkObject,
            _In_ BOOLEAN fReleaseCoreValue)
        {
            return ConvertToFramework(
                pCoreObject,
                __uuidof(U),
                reinterpret_cast<void**>(ppFrameworkObject),
                fReleaseCoreValue);
        }

        static _Check_return_ HRESULT ConvertToFramework(
            _In_opt_ IUnknown* pCoreObject,
            _In_ REFIID iid,
            _Outptr_result_maybenull_ void** ppObject,
            _In_ BOOLEAN fReleaseCoreValue);

        static _Check_return_ HRESULT ConvertToFramework(
            _In_opt_ CDependencyObject* pCoreObject,
            _In_ REFIID iid,
            _Outptr_result_maybenull_ void** ppObject,
            _In_ BOOLEAN fReleaseCoreValue);

        // Convert from xstring_ptr to HSTRING.
        static _Check_return_ HRESULT ConvertToFramework(
            _In_ xstring_ptr coreValue,
            _Outptr_ HSTRING* pFrameworkValue,
            _In_ BOOLEAN fReleaseCoreValue);

        template<class T, class U>
        static _Check_return_ HRESULT ConvertToCore(
            _In_ T frameworkValue,
            U* pCoreValue)
        {
            return E_NOTIMPL;
        }

        // Boxing

        // Box enum value without type resolution.
        static _Check_return_ HRESULT BoxEnumValue(
            _In_ CValue* box,
            _In_ UINT value);

        // Box enum value with type resolution.
        static _Check_return_ HRESULT BoxEnumValue(
            _In_ CValue* box,
            _In_reads_(nLength) const WCHAR* pszValue,
            _In_ XUINT32 nLength,
            _In_ const CClassInfo* pSourceType);

        // Box contents of IReference<T>.  BoxerBuffer is necessary if T needs it to be boxed.
        template<class T>
        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_opt_ IInspectable* value,
            _In_ BoxerBuffer* buffer)
        {
            IFCPTR_RETURN(box);

            if (value)
            {
                T rawValue;
                ctl::ComPtr<wf::IReference<T>> spObjAsRef;

                IFC_RETURN(ctl::do_query_interface(spObjAsRef, value));
                IFC_RETURN(spObjAsRef->get_Value(&rawValue));
                IFC_RETURN(CValueBoxer::BoxValue(box, rawValue, buffer));
            }
            else
            {
                box->SetNull();
            }

            return S_OK;
        }

        // Box contents of IReference<TypeName>.  Needed because TypeName wraps in TypeNamePtr to manage type name lifetime.
        template<> STATIC_SPEC _Check_return_ HRESULT BoxValue<wxaml_interop::TypeName>(
            _In_ CValue* box,
            _In_opt_ IInspectable* value,
            _In_ BoxerBuffer* buffer);

        // Box intrinsic types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ BOOLEAN value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ INT value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ INT64 value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ DOUBLE value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ FLOAT value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_opt_ HSTRING value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        // Box ::Windows::Foundation types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ wf::DateTime value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ wf::TimeSpan value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ wf::Point value,
            _In_ BoxerBuffer* buffer);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ wf::Size value,
            _In_ BoxerBuffer* buffer);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ wf::Rect value,
            _In_ BoxerBuffer* buffer);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_opt_ wf::IUriRuntimeClass* value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        // Box ::Windows::UI types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ wu::Color value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        // Box ::Windows::UI::Text types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ wut::FontWeight value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        // Box ::Windows::UI::Xaml types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml::Duration value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml::CornerRadius value,
            _In_ BoxerBuffer* buffer);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml::Thickness value,
            _In_ BoxerBuffer* buffer);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml::GridLength value,
            _In_ BoxerBuffer* buffer);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_opt_ xaml::IPropertyPath* value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        // Box xaml::Interop types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ wxaml_interop::TypeName value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        // Box xaml::Media types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_opt_ xaml_media::IFontFamily* value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml_media::Matrix value,
            _In_ BoxerBuffer* buffer);

        // Box xaml_media::Animation types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml_animation::KeyTime value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml_animation::RepeatBehavior value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        // Box xaml_media::Media3D types.

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml_media::Media3D::Matrix3D value,
            _In_ BoxerBuffer* buffer);

        // Box xaml::Documents types

        static _Check_return_ HRESULT BoxValue(
            _In_ CValue* box,
            _In_ xaml_docs::TextRange value,
            _In_opt_ BoxerBuffer* buffer = nullptr);

        // Box IInspectable*

        static _Check_return_ HRESULT BoxObjectValue(
            _In_ CValue* box,
            _In_opt_ const CClassInfo* pSourceType,
            _In_opt_ IInspectable* value,
            _In_ BoxerBuffer* buffer,
            _Outptr_result_maybenull_ DependencyObject** ppMOR,
            BOOLEAN bPreserveObjectIdentity = FALSE) noexcept;

        // Unboxing

        // Unbox enum value with type resolution.
        static _Check_return_ HRESULT UnboxEnumValue(
            _In_ const CValue* box,
            _In_opt_ const CClassInfo* pSourceType,
            _Out_ UINT* value);

        // Unbox contents of IReference<T> where T is a struct.
        template<class T>
        static _Check_return_ typename std::enable_if<!std::is_enum<T>::value, HRESULT>::type UnboxValue(
            _In_ const CValue* box,
            _Outptr_ wf::IReference<T>** result)
        {
            return UnboxReferenceValueHelper<T>(
                box,
                &PropertyValue::CreateReference<T>,
                result);
        }

        // Unbox contents of IReference<T> where T is an enum.
        template<class T>
        static _Check_return_ typename std::enable_if<std::is_enum<T>::value, HRESULT>::type UnboxValue(
            _In_ const CValue* box,
            _Outptr_ wf::IReference<T>** result)
        {
            return UnboxReferenceValueHelper<T>(
                box,
                &PropertyValue::CreateEnumReference<T>,
                result);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Outptr_ wf::IReference<bool>** result)
        {
            return UnboxReferenceValueHelper<BOOLEAN, bool>(
                box,
                &PropertyValue::CreateFromBoolean,
                result);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Outptr_ wf::IReference<wf::TimeSpan>** result)
        {
            return UnboxReferenceValueHelper<wf::TimeSpan>(
                box,
                &PropertyValue::CreateFromTimeSpan,
                result);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Outptr_ wf::IReference<wf::Point>** result)
        {
            return UnboxReferenceValueHelper<wf::Point>(
                box,
                &PropertyValue::CreateFromPoint,
                result);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Outptr_ wf::IReference<DOUBLE>** result)
        {
            return UnboxReferenceValueHelper<DOUBLE>(
                box,
                &PropertyValue::CreateFromDouble,
                result);
        }

        template<>
        STATIC_SPEC _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Outptr_ wf::IReference<INT32>** result)
        {
            return UnboxReferenceValueHelper<INT32>(
                box,
                &PropertyValue::CreateFromInt32,
                result);
        }

        // Unbox intrinsic types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ BOOLEAN* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ INT* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ UINT* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ INT64* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ DOUBLE* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ FLOAT* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Outptr_ HSTRING* result);

        // Unbox ::Windows::Foundation types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ wf::DateTime* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ wf::TimeSpan* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ wf::Point* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ wf::Size* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ wf::Rect* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Outptr_result_maybenull_ wf::IUriRuntimeClass** result);

        // Unbox ::Windows::UI types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ wu::Color* result);

        // Unbox ::Windows::UI::Text types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ wut::FontWeight* result);

        // Unbox ::Windows::UI::Xaml types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml::Duration* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml::CornerRadius* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml::Thickness* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml::GridLength* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml::IPropertyPath** result);

        // Unbox xaml::Interop types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ wxaml_interop::TypeName* result);

        // Unbox xaml::Media types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Outptr_result_maybenull_ xaml_media::IFontFamily** result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml_media::Matrix* result);

        // Unbox xaml_media::Animation types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml_animation::KeyTime* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml_animation::RepeatBehavior* result);

        // Unbox xaml_media::Media3D types.

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml_media::Media3D::Matrix3D* result);

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _In_reads_(nFloats) XFLOAT* result,
            _In_ XUINT32 nFloats);

        // Unbox xaml::Documents types

        static _Check_return_ HRESULT UnboxValue(
            _In_ const CValue* box,
            _Out_ xaml_docs::TextRange* result);

        // Unbox IInspectable*

        static _Check_return_ HRESULT UnboxObjectValue(
            _In_ const CValue* box,
            _In_opt_ const CClassInfo* pTargetType,
            _In_ bool targetTypeIsNullable,
            _In_ REFIID riid,
            _Out_ void** result);

        static _Check_return_ HRESULT UnboxObjectValue(
            _In_ const CValue* box,
            _In_opt_ const CClassInfo* pTargetType,
            _In_ REFIID riid,
            _Out_ void** result);

        static _Check_return_ HRESULT UnboxObjectValue(
            _In_ const CValue* box,
            _In_opt_ const CClassInfo* pTargetType,
            _Out_ IInspectable** result);

        static _Check_return_ HRESULT UnboxPropertyObjectValue(
            _In_ const CValue* box,
            _In_ const CDependencyProperty* pDP,
            _Out_ IInspectable** result)
        {
            return UnboxObjectValue(
                box,
                pDP->GetPropertyType(),
                pDP->IsNullable(),
                __uuidof(IInspectable),
                reinterpret_cast<void**>(result));
        }

        static void UnwrapExternalObjectReferenceIfPresent(
            _In_ IInspectable* pObject,
            _Outptr_result_maybenull_ IInspectable** result);

        static _Check_return_ HRESULT UnwrapWeakRef(
            _In_ const CValue* const value,
            _In_ const CDependencyProperty* dp,
            _Outptr_result_maybenull_ CDependencyObject** element);

    private:
        static _Check_return_ HRESULT GetTypeInfoFromCValue(
            _In_ const CValue* box,
            _Out_ const CClassInfo** ppTypeInfo);

        static _Check_return_ HRESULT ResolveEnumValueFromString(
            _In_reads_(nLength) const WCHAR* pszValue,
            _In_ XUINT32 nLength,
            _In_ const CClassInfo* pSourceType,
            _Out_ UINT* value);

        template <typename T, typename U = T>
        static inline _Check_return_ typename std::enable_if<!std::is_enum<T>::value, HRESULT>::type UnboxReferenceValueHelper(
            _In_ const CValue* box,
            _In_ HRESULT (*CreatorFn)(T value, IInspectable** ppResult),
            _Outptr_ wf::IReference<U>** result)
        {
            IFCPTR_RETURN(box);
            IFCPTR_RETURN(result);

            if (!box->IsNull())
            {
                T value;
                ctl::ComPtr<IInspectable> spObj;
                ctl::ComPtr<wf::IReference<U>> spObjAsRef;

                IFC_RETURN(UnboxValue(box, &value));
                IFC_RETURN(CreatorFn(value, &spObj));
                IFC_RETURN(spObj.As(&spObjAsRef));
                IFC_RETURN(spObjAsRef.MoveTo(result));
            }
            else
            {
                *result = nullptr;
            }

            return S_OK;
        }

        template <typename T, typename U = T>
        static inline _Check_return_ typename std::enable_if<std::is_enum<T>::value, HRESULT>::type UnboxReferenceValueHelper(
            _In_ const CValue* box,
            _In_ HRESULT (*CreatorFn)(T value, IInspectable** ppResult),
            _Outptr_ wf::IReference<U>** result)
        {
            IFCPTR_RETURN(box);
            IFCPTR_RETURN(result);

            if (!box->IsNull())
            {
                UINT value;
                ctl::ComPtr<IInspectable> spObj;
                ctl::ComPtr<wf::IReference<U>> spObjAsRef;

                IFC_RETURN(UnboxEnumValue(box, nullptr, &value));
                IFC_RETURN(CreatorFn(static_cast<T>(value), &spObj));
                IFC_RETURN(spObj.As(&spObjAsRef));
                IFC_RETURN(spObjAsRef.MoveTo(result));
            }
            else
            {
                *result = nullptr;
            }

            return S_OK;
        }

        template <typename T>
        static inline _Check_return_ HRESULT UnboxValueHelper(
            _In_ const CValue* box,
            _In_ HRESULT (*FromStringFn)(const xstring_ptr_view&, T*),
            _In_ HRESULT (*FromDOFn)(const CDependencyObject*, T*),
            _Out_ T* result)
        {
            IFCPTR_RETURN(box);
            IFCPTR_RETURN(result);

            if (!box->IsNull())
            {
                switch (box->GetType())
                {
                    case valueSigned:
                        *result = static_cast<T>(box->AsSigned());
                        break;

                    case valueInt64:
                        *result = static_cast<T>(box->AsInt64());
                        break;

                    case valueFloat:
                        *result = static_cast<T>(box->AsFloat());
                        break;

                    case valueDouble:
                        *result = static_cast<T>(box->AsDouble());
                        break;

                    case valueString:
                        {
                            xephemeral_string_ptr strValue;

                            CValueUtil::GetEphemeralString(
                                *box,
                                strValue);

                            IFC_RETURN(FromStringFn(strValue, result));
                        }
                        break;

                    case valueObject:
                        IFC_RETURN(FromDOFn(box->AsObject(), result));
                        break;

                    default:
                        IFC_RETURN(E_UNEXPECTED);
                        break;
                }
            }
            else
            {
                *result = 0;
            }

            return S_OK;
        }
    };
}
