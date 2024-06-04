// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// from local
#include "CValueBoxer.h"
#include "IValueBoxer.h"
#include "BoxerBuffer.h"

// from core
#include "TimeSpan.h"
#include "Duration.h"
#include "RepeatBehavior.h"
#include "ThemeResourceExtension.h"
#include "Type.h"
#include "GridLength.h"
#include "CMatrix.h"
#include <TextRange.h>
#include "MetadataAPI.h"
#include "ActivationAPI.h"
#include "DOPointerCast.h"
#include "CValueUtil.h"

#include "Enums.g.h" // uses GetKnownWinRTBoxFromEnumValue, GetEnumValueFromKnownWinRTBox

// from components
#include "ThemeResource.h"
#include <StringConversions.h>
#include <CColor.h>
#include "CString.h"
#include "Double.h"
#include "Point.h"
#include "Size.h"
#include "Rect.h"
#include "CornerRadius.h"
#include "Thickness.h"
#include "KeyTime.h"
#include "TypeTable.g.h" // uses GetEnumValueTable
#include <TypeNamePtr.h>
#include <clock.h>

// from fx
#include "Synonyms.g.h"
#include "StaticStore.h"
#include "JoltCollections.h" // uses ValueTypeView
#include "ExternalObjectReference_partial.h"
#include "ThemeResourceExpression.h"
#include "comInstantiation.h" // uses ctl::make
#include "CoreImports.h"

#include "DependencyObject.h" // needed by FontFamily.g.h, PropertyPath.g.h
#include "FontFamily.g.h"
#include "PropertyPath.g.h"

// from other
#include <tchar.h> // TODO: is _tcsto* what we use for parsing strings?
#include "DoubleUtil.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT CValueBoxer::ConvertToFramework(
    _In_opt_ IUnknown* pCoreObject,
    _In_ REFIID iid,
    _Outptr_result_maybenull_ void** ppObject,
    _In_ BOOLEAN fReleaseCoreValue)
{
    auto cleanupGuard = wil::scope_exit([&]
    {
        if (fReleaseCoreValue)
        {
            ReleaseInterface(pCoreObject);
        }
    });

    if (pCoreObject)
    {
        IFC_RETURN(pCoreObject->QueryInterface(iid, ppObject));
    }
    else
    {
        *ppObject = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::ConvertToFramework(
    _In_opt_ CDependencyObject* pCoreObject,
    _In_ REFIID iid,
    _Outptr_result_maybenull_ void** ppObject,
    _In_ BOOLEAN fReleaseCoreValue)
{
    auto cleanupGuard = wil::scope_exit([&]
    {
        if (fReleaseCoreValue)
        {
            ReleaseInterface(pCoreObject);
        }
    });

    if (pCoreObject)
    {
        ctl::ComPtr<DependencyObject> spPeer;
        IFC_RETURN(DXamlServices::GetPeer(pCoreObject, &spPeer));
        IFC_RETURN(ctl::iinspectable_cast(spPeer.Get())->QueryInterface(iid, ppObject));
    }
    else
    {
        *ppObject = nullptr;
    }

    return S_OK;
}

// Convert from xstring_ptr to HSTRING.
_Check_return_ HRESULT CValueBoxer::ConvertToFramework(
    _In_ xstring_ptr coreValue,
    _Outptr_ HSTRING* pFrameworkValue,
    _In_ BOOLEAN fReleaseCoreValue)
{
    xruntime_string_ptr strPromoted;
    IFC_RETURN(coreValue.Promote(&strPromoted));
    *pFrameworkValue = strPromoted.DetachHSTRING();

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxEnumValue(
    _In_ CValue* box,
    _In_ UINT value)
{
    IFCPTR_RETURN(box);
    box->SetEnum(value);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxEnumValue(
    _In_ CValue* box,
    _In_reads_(nLength) const WCHAR* pszValue,
    _In_ XUINT32 nLength,
    _In_ const CClassInfo* pSourceType)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(pszValue);
    IFCEXPECT_RETURN(nLength);
    IFCEXPECT_RETURN(pSourceType);

    UINT value = 0;

    IFC_RETURN(ResolveEnumValueFromString(
        pszValue,
        nLength,
        pSourceType,
        &value));

    IFC_RETURN(CValueBoxer::BoxEnumValue(box, value));

    return S_OK;
}

template<>
_Check_return_ HRESULT CValueBoxer::BoxValue<wxaml_interop::TypeName>(
    _In_ CValue* box,
    _In_opt_ IInspectable* value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    if (value)
    {
        TypeNamePtr rawValue;
        ctl::ComPtr<wf::IReference<wxaml_interop::TypeName>> spObjAsRef;

        IFC_RETURN(ctl::do_query_interface(spObjAsRef, value));
        IFC_RETURN(spObjAsRef->get_Value(rawValue.ReleaseAndGetAddressOf()));
        IFC_RETURN(CValueBoxer::BoxValue(box, rawValue.Get()));
    }
    else
    {
        box->SetNull();
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ BOOLEAN value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetBool(!!value);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ INT value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetSigned(value);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ INT64 value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetInt64(value);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ DOUBLE value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetDouble(value);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ FLOAT value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetFloat(value);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_opt_ HSTRING value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);

    if (value != nullptr)
    {
        xstring_ptr strValue;

        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(
            value,
            &strValue));

        box->SetString(std::move(strValue));
    }
    else
    {
        box->SetString(xstring_ptr::NullString());
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ wf::DateTime value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetDateTime(value);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ wf::TimeSpan value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetTimeSpan(value);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ wf::Point value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    buffer->SetPoint(
        value.X,
        value.Y);

    box->WrapPoint(buffer->GetPoint());

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ wf::Size value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    buffer->SetSize(
        value.Width,
        value.Height);

    box->WrapSize(buffer->GetSize());

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ wf::Rect value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    buffer->SetRect(
        value.X,
        value.Y,
        value.Width,
        value.Height);

    box->WrapRect(buffer->GetRect());

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_opt_ wf::IUriRuntimeClass* value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);

    wrl_wrappers::HString strAbsoluteUri;

    if (value)
    {
        IFC_RETURN(value->get_AbsoluteUri(strAbsoluteUri.GetAddressOf()));
    }

    IFC_RETURN(CValueBoxer::BoxValue(box, strAbsoluteUri.Get()));

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ wu::Color value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetColor(((UINT)value.A << 24) | ((UINT)value.R << 16) | ((UINT)value.G << 8) | (UINT)value.B);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ wut::FontWeight value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    box->SetEnum(value.Weight);
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml::Duration value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);

    switch (value.Type)
    {
        case xaml::DurationType_Automatic:
        case xaml::DurationType_Forever:
            {
                xstring_ptr strValue;

                IFC_RETURN(DurationToString(
                    static_cast<DirectUI::DurationType>(value.Type),
                    0,  // This is not used, so save the calculation -- GetSecondsFromTicks(value.TimeSpan.Duration),
                    strValue));

                box->SetString(std::move(strValue));
            }
            break;

        case xaml::DurationType_TimeSpan:
            IFC_RETURN(CValueBoxer::BoxValue(box, value.TimeSpan));
            break;

        default:
            IFC_RETURN(E_FAIL);
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml::CornerRadius value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    buffer->SetCornerRadius(
        static_cast<FLOAT>(value.BottomLeft),
        static_cast<FLOAT>(value.BottomRight),
        static_cast<FLOAT>(value.TopLeft),
        static_cast<FLOAT>(value.TopRight));

    box->WrapCornerRadius(buffer->GetCornerRadius());

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml::Thickness value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    buffer->SetThickness(
        static_cast<FLOAT>(value.Left),
        static_cast<FLOAT>(value.Top),
        static_cast<FLOAT>(value.Right),
        static_cast<FLOAT>(value.Bottom));

    box->WrapThickness(buffer->GetThickness());

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml::GridLength value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    buffer->SetGridLength(
        static_cast<DirectUI::GridUnitType>(value.GridUnitType),
        static_cast<XFLOAT>(value.Value));

    box->WrapGridLength(buffer->GetGridLength());

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_opt_ xaml::IPropertyPath* value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);

    wrl_wrappers::HString strPath;

    if (value)
    {
        IFC_RETURN(value->get_Path(strPath.GetAddressOf()));
    }

    IFC_RETURN(CValueBoxer::BoxValue(box, strPath.Get()));

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ wxaml_interop::TypeName value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);

    if (value.Name)
    {
        const CClassInfo* type = nullptr;
        IFC_RETURN(MetadataAPI::GetClassInfoByTypeName(value, &type));
        box->SetTypeHandle(type->GetIndex());
    }
    else
    {
        box->SetNull();
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_opt_ IFontFamily* value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);

    wrl_wrappers::HString strSource;

    if (value)
    {
        IFC_RETURN(value->get_Source(strSource.GetAddressOf()));
    }

    IFC_RETURN(CValueBoxer::BoxValue(box, strSource.Get()));

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml_media::Matrix value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    buffer->SetMatrix(
        static_cast<FLOAT>(value.M11),
        static_cast<FLOAT>(value.M12),
        static_cast<FLOAT>(value.M21),
        static_cast<FLOAT>(value.M22),
        static_cast<FLOAT>(value.OffsetX),
        static_cast<FLOAT>(value.OffsetY));

    box->WrapFloatArray(
        6,
        reinterpret_cast<FLOAT*>(const_cast<XMATRIX*>(buffer->GetMatrix())));

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml_animation::KeyTime value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFC_RETURN(CValueBoxer::BoxValue(box, value.TimeSpan));
    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml_animation::RepeatBehavior value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);

    switch (value.Type)
    {
        case xaml_animation::RepeatBehaviorType_Count:
        case xaml_animation::RepeatBehaviorType_Forever:
            {
                xstring_ptr strValue;

                IFC_RETURN(RepeatBehaviorToString(
                    static_cast<DirectUI::RepeatBehaviorType>(value.Type),
                    0,  // This is not used, so save the calculation -- GetSecondsFromTicks(value.Duration.Duration),
                    static_cast<float>(value.Count),
                    strValue));

                box->SetString(std::move(strValue));
            }
            break;

        case xaml_animation::RepeatBehaviorType_Duration:
            IFC_RETURN(CValueBoxer::BoxValue(box, value.Duration));
            break;

        default:
            IFC_RETURN(E_FAIL);
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml_media::Media3D::Matrix3D value,
    _In_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);

    buffer->SetMatrix3D(
        static_cast<XFLOAT>(value.M11),
        static_cast<XFLOAT>(value.M12),
        static_cast<XFLOAT>(value.M13),
        static_cast<XFLOAT>(value.M14),
        static_cast<XFLOAT>(value.M21),
        static_cast<XFLOAT>(value.M22),
        static_cast<XFLOAT>(value.M23),
        static_cast<XFLOAT>(value.M24),
        static_cast<XFLOAT>(value.M31),
        static_cast<XFLOAT>(value.M32),
        static_cast<XFLOAT>(value.M33),
        static_cast<XFLOAT>(value.M34),
        static_cast<XFLOAT>(value.OffsetX),
        static_cast<XFLOAT>(value.OffsetY),
        static_cast<XFLOAT>(value.OffsetZ),
        static_cast<XFLOAT>(value.M44));

    box->WrapFloatArray(
        16,
        reinterpret_cast<FLOAT*>(const_cast<XMATRIX3D*>(buffer->GetMatrix3D())));

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxValue(
    _In_ CValue* box,
    _In_ xaml_docs::TextRange value,
    _In_opt_ BoxerBuffer* buffer)
{
    IFCPTR_RETURN(box);

    box->Set<valueTextRange>({ value.StartIndex, value.Length });

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::BoxObjectValue(
    _In_ CValue* box,
    _In_opt_ const CClassInfo* pSourceType,
    _In_opt_ IInspectable* value,
    _In_ BoxerBuffer* buffer,
    _Outptr_result_maybenull_ DependencyObject** ppMOR,
    _In_opt_ BOOLEAN bPreserveObjectIdentity) noexcept
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(buffer);
    IFCPTR_RETURN(ppMOR);

    wf::PropertyType valueType = wf::PropertyType_Empty;
    ctl::ComPtr<DependencyObject> spMOR;
    ctl::ComPtr<wf::IPropertyValue> spValueAsPV = ctl::query_interface_cast<wf::IPropertyValue>(value);
    ctl::ComPtr<IInspectable> spValueAsInsp;

    *ppMOR = nullptr;

    if (spValueAsPV)
    {
        IFC_RETURN(spValueAsPV->get_Type(&valueType));

        if (valueType != wf::PropertyType_Empty)
        {
            spValueAsInsp = value;
        }

        if (pSourceType == nullptr || (!bPreserveObjectIdentity && pSourceType->GetIndex() == KnownTypeIndex::Object))
        {
            // Try to resolve a more specific type.
            IFC_RETURN(MetadataAPI::GetClassInfoFromWinRTPropertyType(spValueAsPV.Get(), valueType, &pSourceType));
        }
    }
    else if (value)
    {
        spValueAsInsp = value;
    }

    if (!spValueAsInsp)
    {
        box->SetNull();
        return S_OK;
    }

    if (pSourceType == nullptr || (!bPreserveObjectIdentity && pSourceType->GetIndex() == KnownTypeIndex::Object))
    {
        // Try to resolve a more specific type.
        IFC_RETURN(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(value, &pSourceType));
    }

    if (pSourceType->IsEnum())
    {
        if (valueType == wf::PropertyType_Int32)
        {
            INT nValue = 0;
            IFC_RETURN(spValueAsPV->GetInt32(&nValue));
            IFC_RETURN(CValueBoxer::BoxEnumValue(box, static_cast<UINT>(nValue)));
            return S_OK;
        }
        else if (valueType == wf::PropertyType_UInt32)
        {
            UINT nValue = 0;
            IFC_RETURN(spValueAsPV->GetUInt32(&nValue));
            IFC_RETURN(CValueBoxer::BoxEnumValue(box, nValue));
            return S_OK;
        }
        else if (valueType == wf::PropertyType_OtherType)
        {
            UINT nEnumValue = 0;

            // If we're unable to unbox the enum, we will treat the value as a regular IInspectable like we did in Windows 8.1.
            if (SUCCEEDED(GetEnumValueFromKnownWinRTBox(value, pSourceType, &nEnumValue)))
            {
                box->SetEnum(nEnumValue, pSourceType->GetIndex());
                return S_OK;
            }
        }
    }

    ASSERT(valueType != wf::PropertyType_Empty || !spValueAsPV);

    switch (pSourceType->m_nIndex)
    {
        case KnownTypeIndex::Boolean:
            {
                BOOLEAN bValue = 0;
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFCFAILFAST(spValueAsPV->GetBoolean(&bValue));
                IFC_RETURN(CValueBoxer::BoxValue(box, bValue, buffer));
            }
            break;

        case KnownTypeIndex::Int32:
            switch (valueType)
            {
                case wf::PropertyType_UInt8:
                    {
                        UINT8 nValue = 0;
                        IFC_RETURN(spValueAsPV->GetUInt8(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_Int16:
                    {
                        INT16 nValue = 0;
                        IFC_RETURN(spValueAsPV->GetInt16(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_UInt16:
                    {
                        UINT16 nValue = 0;
                        IFC_RETURN(spValueAsPV->GetUInt16(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_Int32:
                    {
                        INT nValue = 0;
                        IFC_RETURN(spValueAsPV->GetInt32(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, nValue, buffer));
                    }
                    break;

                case wf::PropertyType_UInt32:
                    {
                        UINT nValue = 0;
                        IFC_RETURN(spValueAsPV->GetUInt32(&nValue));
                        //TODO: this still needs to be updated if it's meant to be BOOLEAN or UINT
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_Double:
                    {
                        DOUBLE nValue = 0;
                        IFC_RETURN(spValueAsPV->GetDouble(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, nValue, buffer));
                    }
                    break;

                default:
                    IFC_RETURN(E_FAIL);
                    break;
            }
            break;

        case KnownTypeIndex::Int64:
            switch (valueType)
            {
                case wf::PropertyType_UInt8:
                    {
                        UINT8 nValue = 0;
                        IFC_RETURN(spValueAsPV->GetUInt8(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT64>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_Int16:
                    {
                        INT16 nValue = 0;
                        IFC_RETURN(spValueAsPV->GetInt16(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT64>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_UInt16:
                    {
                        UINT16 nValue = 0;
                        IFC_RETURN(spValueAsPV->GetUInt16(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT64>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_Int32:
                    {
                        INT nValue = 0;
                        IFC_RETURN(spValueAsPV->GetInt32(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT64>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_UInt32:
                    {
                        UINT nValue = 0;
                        IFC_RETURN(spValueAsPV->GetUInt32(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT64>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_Int64:
                    {
                        INT64 nValue = 0;
                        IFC_RETURN(spValueAsPV->GetInt64(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT64>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_UInt64:
                    {
                        UINT64 nValue = 0;
                        IFC_RETURN(spValueAsPV->GetUInt64(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, static_cast<INT64>(nValue), buffer));
                    }
                    break;

                case wf::PropertyType_Double:
                    {
                        DOUBLE nValue = 0;
                        IFC_RETURN(spValueAsPV->GetDouble(&nValue));
                        IFC_RETURN(CValueBoxer::BoxValue(box, nValue, buffer));
                    }
                    break;

                default:
                    IFC_RETURN(E_FAIL);
                    break;
            }
            break;

        case KnownTypeIndex::Float:
            {
                FLOAT nValue = 0;
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFC_RETURN(spValueAsPV->GetSingle(&nValue));

                // Core deals with floats.
                box->SetFloat(nValue);
            }
            break;

        case KnownTypeIndex::Double:
            {
                DOUBLE nValue = 0;
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFC_RETURN(spValueAsPV->GetDouble(&nValue));
                IFC_RETURN(CValueBoxer::BoxValue(box, nValue, buffer));
            }
            break;

        case KnownTypeIndex::String:
            {
                wrl_wrappers::HString strValue;
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFC_RETURN(spValueAsPV->GetString(strValue.GetAddressOf()));
                IFC_RETURN(CValueBoxer::BoxValue(box, strValue.Get(), buffer));
            }
            break;

        case KnownTypeIndex::Point:
            {
                wf::Point pointValue = {};
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFC_RETURN(spValueAsPV->GetPoint(&pointValue));
                IFC_RETURN(CValueBoxer::BoxValue(box, pointValue, buffer));
            }
            break;

        case KnownTypeIndex::Rect:
            {
                wf::Rect rectValue = {};
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFC_RETURN(spValueAsPV->GetRect(&rectValue));
                IFC_RETURN(CValueBoxer::BoxValue(box, rectValue, buffer));
            }
            break;

        case KnownTypeIndex::Size:
            {
                wf::Size sizeValue = {};
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFC_RETURN(spValueAsPV->GetSize(&sizeValue));
                IFC_RETURN(CValueBoxer::BoxValue(box, sizeValue, buffer));
            }
            break;

        case KnownTypeIndex::TimeSpan:
            {
                wf::TimeSpan timeSpanValue = {};
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFC_RETURN(spValueAsPV->GetTimeSpan(&timeSpanValue));
                IFC_RETURN(CValueBoxer::BoxValue(box, timeSpanValue, buffer));
            }
            break;

        case KnownTypeIndex::DateTime:
            {
                wf::DateTime dateTimeValue = {};
                IFCCHECK_RETURN(spValueAsPV != nullptr);
                IFC_RETURN(spValueAsPV->GetDateTime(&dateTimeValue));
                IFC_RETURN(CValueBoxer::BoxValue(box, dateTimeValue, buffer));
            }
            break;

        case KnownTypeIndex::TypeName:
            IFC_RETURN(CValueBoxer::BoxValue<wxaml_interop::TypeName>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::CornerRadius:
            IFC_RETURN(CValueBoxer::BoxValue<xaml::CornerRadius>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::Duration:
            IFC_RETURN(CValueBoxer::BoxValue<xaml::Duration>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::FontWeight:
            IFC_RETURN(CValueBoxer::BoxValue<wut::FontWeight>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::GridLength:
            IFC_RETURN(CValueBoxer::BoxValue<xaml::GridLength>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::Thickness:
            IFC_RETURN(CValueBoxer::BoxValue<xaml::Thickness>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::KeyTime:
            IFC_RETURN(CValueBoxer::BoxValue<xaml_animation::KeyTime>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::RepeatBehavior:
            IFC_RETURN(CValueBoxer::BoxValue<xaml_animation::RepeatBehavior>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::Color:
            IFC_RETURN(CValueBoxer::BoxValue<wu::Color>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::Matrix:
            IFC_RETURN(CValueBoxer::BoxValue<xaml_media::Matrix>(box, spValueAsInsp.Get(), buffer));
            break;

        case KnownTypeIndex::Matrix3D:
            IFC_RETURN(CValueBoxer::BoxValue<xaml_media::Media3D::Matrix3D>(box, spValueAsInsp.Get(), buffer));
            break;


        case KnownTypeIndex::TextRange:
            {
                IFC_RETURN(CValueBoxer::BoxValue<xaml_docs::TextRange>(box, spValueAsInsp.Get(), buffer));
            }
            break;

        case KnownTypeIndex::Uri:
            {
                ctl::ComPtr<wf::IUriRuntimeClass> spObj = spValueAsInsp.AsOrNull<wf::IUriRuntimeClass>();
                IFC_RETURN(CValueBoxer::BoxValue(box, spObj.Get(), buffer));
            }
            break;

        case KnownTypeIndex::PropertyPath:
            {
                ctl::ComPtr<IPropertyPath> spObj = spValueAsInsp.AsOrNull<IPropertyPath>();
                IFC_RETURN(CValueBoxer::BoxValue(box, spObj.Get(), buffer));
            }
            break;

        default:
            IFC_RETURN(ExternalObjectReference::ConditionalWrap(spValueAsInsp.Get(), &spMOR));
            box->SetObjectAddRef(spMOR->GetHandle());
            IFC_RETURN(spMOR.MoveTo(ppMOR));
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxEnumValue(
    _In_ const CValue* box,
    _In_opt_ const CClassInfo* pSourceType,
    _Out_ UINT* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    switch (box->GetType())
    {
        case valueEnum:
        case valueEnum8:
            *value = box->AsEnum();
            break;

        case valueSigned:
            *value = box->AsSigned();
            break;

        case valueString:
            {
                IFCEXPECT_RETURN(pSourceType);
                XUINT32 cString = 0;
                const WCHAR* pString = box->AsEncodedString().GetBufferAndCount(&cString);
                IFC_RETURN(ResolveEnumValueFromString(pString, cString, pSourceType, value));
            }
            break;

        case valueNull:
            *value = 0;
            break;

        default:
            // unknown box type
            IFC_RETURN(E_UNEXPECTED);
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ BOOLEAN* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueBool:
                *value = !!box->AsBool();
                break;

            case valueString:
                {
                    XUINT32 cString = 0;
                    const WCHAR* pString = box->AsEncodedString().GetBufferAndCount(&cString);
                    // compare ignoring case
                    *value = !!_wcsnicmp(L"FALSE", pString, cString);
                }
                break;

            case valueEnum:
            case valueEnum8:
                // When setting Setter.Value to a BOOLEAN, it gets marshaled as an enum in the core.
                *value = box->AsEnum() != 0;
                break;
            case valueObject:
                // Boolean objects stored in resource dictionaries are CEnumerated
                *value = checked_cast<CEnumerated>(box->AsObject())->m_nValue != 0;
                break;
            default:
                // unknown box type
                IFC_RETURN(E_UNEXPECTED);
                break;
        }
    }
    else
    {
        *value = FALSE;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ INT* value)
{
    return UnboxValueHelper<INT>(
        box,
        [](const xstring_ptr_view& fromString, INT* value) -> HRESULT
        {
            *value = _tcstol(fromString.GetBuffer(), nullptr, 10);
            return S_OK;
        },
        [](const CDependencyObject* fromDO, INT* value) -> HRESULT
        {
            const CInt32* pInt = do_pointer_cast<CInt32>(fromDO);
            *value = pInt->m_iValue;
            return S_OK;
        },
        value);
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ UINT* value)
{
    return UnboxValueHelper<UINT>(
        box,
        [](const xstring_ptr_view& fromString, UINT* value) -> HRESULT
        {
            *value = wcstoul(fromString.GetBuffer(), nullptr, 10);
            return S_OK;
        },
        [](const CDependencyObject* fromDO, UINT* value) -> HRESULT
        {
            const CInt32* pInt = do_pointer_cast<CInt32>(fromDO);
            *value = static_cast<UINT>(pInt->m_iValue);
            return S_OK;
        },
        value);
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ INT64* value)
{
    return UnboxValueHelper<INT64>(
        box,
        [](const xstring_ptr_view& fromString, INT64* value) -> HRESULT
        {
            *value = _wcstoi64(fromString.GetBuffer(), nullptr, 10);
            return S_OK;
        },
        [](const CDependencyObject* fromDO, INT64* value) -> HRESULT
        {
            return E_UNEXPECTED;
        },
        value);
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ DOUBLE* value)
{
    return UnboxValueHelper<DOUBLE>(
        box,
        [](const xstring_ptr_view& fromString, DOUBLE* value) -> HRESULT
        {
            *value = _tcstod(fromString.GetBuffer(), nullptr);
            return S_OK;
        },
        [](const CDependencyObject* fromDO, DOUBLE* value) -> HRESULT
        {
            const CDouble* pDouble = do_pointer_cast<CDouble>(fromDO);
            *value = static_cast<DOUBLE>(pDouble->m_eValue);
            return S_OK;
        },
        value);
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ FLOAT* value)
{
    return UnboxValueHelper<FLOAT>(
        box,
        [](const xstring_ptr_view& fromString, FLOAT* value) -> HRESULT
        {
            *value = static_cast<FLOAT>(_tcstod(fromString.GetBuffer(), nullptr));
            return S_OK;
        },
        [](const CDependencyObject* fromDO, FLOAT* value) -> HRESULT
        {
            const CDouble* pDouble = do_pointer_cast<CDouble>(fromDO);
            *value = static_cast<FLOAT>(pDouble->m_eValue);
            return S_OK;
        },
        value);
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Outptr_ HSTRING* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    xruntime_string_ptr strRuntimeValue;

    switch (box->GetType())
    {
        case valueString:
            // Nulls should be interpreted as empty strings.
            IFC_RETURN(CValueUtil::GetRuntimeString(
                *box,
                strRuntimeValue));
            break;

        case valueObject:
            {
                CString* pString = do_pointer_cast<CString>(box->AsObject());
                IFC_RETURN(pString->m_strString.Promote(&strRuntimeValue));
            }
            break;

        default:
            IFCEXPECT_RETURN(box->IsNull());
            break;
    }

    *value = strRuntimeValue.DetachHSTRING();

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ wf::DateTime* value)
{
    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueIInspectable:
                return IValueBoxer::UnboxValue(box->AsIInspectable(), value);

            case valueDateTime:
                return box->GetDateTime(*value);

            default:
                return E_NOTIMPL;
        }
    }
    else
    {
        value->UniversalTime = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ wf::TimeSpan* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueDouble:
                *value = TimeSpanUtil::FromSeconds(box->AsDouble());
                break;

            case valueString:
                {
                    IFC_RETURN(TimeSpanFromString(box->AsString(), *value));
                }
                break;

            case valueObject:
                {
                    CTimeSpan* pTimeSpan = checked_cast<CTimeSpan>(box->AsObject());
                    *value = TimeSpanUtil::FromSeconds(pTimeSpan->m_rTimeSpan);
                }
                break;

            case valueTimeSpan:
                IFC_RETURN(box->GetTimeSpan(*value));
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }
    }
    else
    {
        value->Duration = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ wf::Point* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        XPOINTF* pCorePoint = nullptr;
        XPOINTF point = {};

        switch (box->GetType())
        {
            case valuePoint:
                pCorePoint = box->AsPoint();
                break;

            case valueString:
                {
                    XUINT32 cString = 0;
                    const WCHAR* pString = box->AsEncodedString().GetBufferAndCount(&cString);
                    IFC_RETURN(ArrayFromString(cString, pString, &cString, &pString, 2, (XFLOAT*)&point));
                    pCorePoint = &point;
                }
                break;

            case valueObject:
                {
                    CPoint* pPoint = do_pointer_cast<CPoint>(box->AsObject());
                    pCorePoint = &pPoint->m_pt;
                }
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }

        IFCPTR_RETURN(pCorePoint);

        value->X = pCorePoint->x;
        value->Y = pCorePoint->y;
    }
    else
    {
        value->X = 0;
        value->Y = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ wf::Size* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        XSIZEF* pCoreSize = nullptr;
        XSIZEF size = {};

        switch (box->GetType())
        {
            case valueSize:
                pCoreSize = box->AsSize();
                break;

            case valueString:
                {
                    XUINT32 cString = 0;
                    const WCHAR* pString = box->AsEncodedString().GetBufferAndCount(&cString);
                    IFC_RETURN(ArrayFromString(cString, pString, &cString, &pString, 2, (XFLOAT*)&size));
                    pCoreSize = &size;
                }
                break;

            case valueObject:
                {
                    CSize* pSize = do_pointer_cast<CSize>(box->AsObject());
                    pCoreSize = &pSize->m_size;
                }
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }

        IFCPTR_RETURN(pCoreSize);

        value->Height = pCoreSize->height;
        value->Width = pCoreSize->width;
    }
    else
    {
        value->Height = 0;
        value->Width = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ wf::Rect* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        XRECTF* pCoreRect = nullptr;
        XRECTF rect = {};

        switch (box->GetType())
        {
            case valueRect:
                pCoreRect = box->AsRect();
                break;

            case valueString:
                {
                    XUINT32 cString = 0;
                    const WCHAR* pString = box->AsEncodedString().GetBufferAndCount(&cString);
                    IFC_RETURN(ArrayFromString(cString, pString, &cString, &pString, 4, (XFLOAT*)&rect));
                    pCoreRect = &rect;
                }
                break;

            case valueObject:
                {
                    CRect* pRect = do_pointer_cast<CRect>(box->AsObject());
                    pCoreRect = &pRect->m_rc;
                }
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }

        IFCPTR_RETURN(pCoreRect);

        value->X = pCoreRect->X;
        value->Y = pCoreRect->Y;
        value->Width = pCoreRect->Width;
        value->Height = pCoreRect->Height;
    }
    else
    {
        value->X = 0;
        value->Y = 0;
        value->Width = 0;
        value->Height = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Outptr_result_maybenull_ wf::IUriRuntimeClass** value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    xruntime_string_ptr strUri;

    *value = nullptr;

    if (valueString == box->GetType())
    {
        IFC_RETURN(CValueUtil::GetRuntimeString(
            *box,
            strUri));
    }

    if (!strUri.IsNullOrEmpty())
    {
        ctl::ComPtr<wf::IUriRuntimeClassFactory> spUriFactory;

        IFC_RETURN(StaticStore::GetUriFactory(&spUriFactory));

        // Try to create a URI out of the string
        if (SUCCEEDED(spUriFactory->CreateUri(strUri.GetHSTRING(), value)))
        {
            return S_OK;
        }

        // If we failed to create a URI, then we have a fragment. We need to combine it with a base URI.
        xstring_ptr xstrBaseUri;
        CCoreServices* coreServices = DXamlServices::GetHandle();

        IFC_RETURN(CoreImports::CoreServices_GetBaseUri(
                coreServices,
                &xstrBaseUri));

        xruntime_string_ptr strCombinedUri;

        IFC_RETURN(CoreImports::CoreServices_CombineResourceUri(
                coreServices,
                xstrBaseUri,
                strUri,
                &strCombinedUri));

        IFC_RETURN(spUriFactory->CreateUri(strCombinedUri.GetHSTRING(), value));
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ wu::Color* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        UINT32 nValue = 0;

        switch (box->GetType())
        {
            case valueColor:
                nValue = box->AsColor();
                break;

            case valueString:
                IFC_RETURN(CColor::ColorFromString(box->AsString(), &nValue));
                break;

            case valueObject:
                {
                    CColor* pColor = do_pointer_cast<CColor>(box->AsObject());

                    if (pColor != nullptr)
                    {
                        nValue = pColor->m_rgb;
                    }
                    else
                    {
                        IFC_RETURN(E_UNEXPECTED);
                    }
                }
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }

        value->B = (nValue & 0xff);
        nValue = nValue >> 8;
        value->G = (nValue & 0xff);
        nValue = nValue >> 8;
        value->R = (nValue & 0xff);
        nValue = nValue >> 8;
        value->A = (nValue & 0xff);
    }
    else
    {
        value->A = 0;
        value->R = 0;
        value->G = 0;
        value->B = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ wut::FontWeight* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueEnum:
            case valueEnum8:
                value->Weight = static_cast<UINT16>(box->AsEnum());
                break;

            case valueObject:
                {
                    CEnumerated* pEnumerated = do_pointer_cast<CEnumerated>(box->AsObject());
                    value->Weight = static_cast<UINT16>(pEnumerated->m_nValue);
                }
                break;

            default:
                IFCEXPECT_RETURN(false);
                break;
        }
    }
    else
    {
        value->Weight = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml::Duration* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueDouble:
                IFC_RETURN(UnboxValue(box, &value->TimeSpan));
                value->Type = xaml::DurationType_TimeSpan;
                break;

            case valueString:
                {
                    DirectUI::DurationType type = DirectUI::DurationType::Automatic;
                    double durationInSec = 0.0;

                    IFC_RETURN(DurationFromString(
                        box->As<valueString>(),
                        &type,
                        &durationInSec));

                    value->Type = static_cast<xaml::DurationType>(type);
                    value->TimeSpan = TimeSpanUtil::FromSeconds(durationInSec);
                }
                break;

            case valueObject:
                {
                    CDuration* duration = do_pointer_cast<CDuration>(box->AsObject());
                    value->Type = static_cast<xaml::DurationType>(duration->GetDurationType());
                    value->TimeSpan = TimeSpanUtil::FromSeconds(duration->GetTimeSpanInSec());
                }
                break;

            case valueVO:
                {
                    ASSERT(box->As<valueVO>()->GetTypeIndex() == DurationVO::s_typeIndex);
                    const DurationVO& vo = static_cast<const DurationVO::Wrapper*>(box->As<valueVO>())->Value();
                    value->Type = static_cast<xaml::DurationType>(vo.GetDurationType());
                    value->TimeSpan = TimeSpanUtil::FromSeconds(vo.GetTimeSpanInSec());
                }
                break;

            default:
                IFC_RETURN(E_FAIL);
                break;
        }
    }
    else
    {
        value->Type = xaml::DurationType_Automatic;
        value->TimeSpan.Duration = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml::CornerRadius* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        XCORNERRADIUS* pCoreCornerRadius = nullptr;
        XCORNERRADIUS cornerRadius = {};

        switch (box->GetType())
        {
            case valueCornerRadius:
                pCoreCornerRadius = box->AsCornerRadius();
                break;

            case valueString:
                {
                    XUINT32 cString = 0;
                    const WCHAR* pString = box->AsEncodedString().GetBufferAndCount(&cString);
                    IFC_RETURN(CCornerRadius::CornerRadiusFromString(cString, pString, &cornerRadius));
                    pCoreCornerRadius = &cornerRadius;
                }
                break;

            case valueObject:
                {
                    CCornerRadius* pCornerRadius = do_pointer_cast<CCornerRadius>(box->AsObject());
                    pCoreCornerRadius = &pCornerRadius->m_cornerRadius;
                }
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }

        IFCPTR_RETURN(pCoreCornerRadius);

        value->TopLeft = pCoreCornerRadius->topLeft;
        value->TopRight = pCoreCornerRadius->topRight;
        value->BottomLeft = pCoreCornerRadius->bottomLeft;
        value->BottomRight = pCoreCornerRadius->bottomRight;
    }
    else
    {
        value->TopLeft = 0;
        value->TopRight = 0;
        value->BottomRight = 0;
        value->BottomLeft = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml::Thickness* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        XTHICKNESS* pCoreThickness = nullptr;
        XTHICKNESS thickness = {};

        switch (box->GetType())
        {
            case valueThickness:
                pCoreThickness = box->AsThickness();
                break;

            // even though we are no longer marshalling as string, there are cases where
            // we are animating an object keyframe. That property is typed as valueAny and thus
            // the parser keeps it as a string. When we have a custom property of type Thickness
            // and animate using that object keyframe, there was no reason to ever cast it to thickness
            // and thus the marshalling layer needs to be able to handle it
            case valueString:
                {
                    IFC_RETURN(ThicknessFromString(box->AsString(), &thickness));
                    pCoreThickness = &thickness;
                }
                break;

            case valueObject:
                {
                    CThickness* pThickness = do_pointer_cast<CThickness>(box->AsObject());
                    pCoreThickness = &pThickness->m_thickness;
                }
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }

        IFCPTR_RETURN(pCoreThickness);

        value->Left = pCoreThickness->left;
        value->Top = pCoreThickness->top;
        value->Right = pCoreThickness->right;
        value->Bottom = pCoreThickness->bottom;
    }
    else
    {
        value->Left = 0;
        value->Top = 0;
        value->Right = 0;
        value->Bottom = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml::GridLength* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        XGRIDLENGTH* pCoreGridLength = nullptr;
        XGRIDLENGTH gridLength = {};

        switch (box->GetType())
        {
            case valueGridLength:
                pCoreGridLength = box->AsGridLength();
                break;

            case valueString:
                {
                    IFC_RETURN(GridLengthFromString(box->AsString(), &gridLength));
                    pCoreGridLength = &gridLength;
                }
                break;

            case valueObject:
                {
                    CGridLength* pGridLength = do_pointer_cast<CGridLength>(box->AsObject());
                    pCoreGridLength = &pGridLength->m_gridLength;
                }
                break;

            case valueDouble:
            case valueFloat:
                {
                    gridLength.type = DoubleUtil::IsNaN(box->AsDouble()) ?
                        GridUnitType::Auto :
                        GridUnitType::Pixel;
                    gridLength.value = static_cast<float>(box->AsDouble());
                    pCoreGridLength = &gridLength;
                }
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }

        IFCPTR_RETURN(pCoreGridLength);

        value->GridUnitType = (xaml::GridUnitType)pCoreGridLength->type;
        value->Value = pCoreGridLength->value;
    }
    else
    {
        value->GridUnitType = xaml::GridUnitType::GridUnitType_Auto;
        value->Value = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml::IPropertyPath** value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        wrl_wrappers::HString strPath;
        ctl::ComPtr<DependencyObject> spPeer;
        ctl::ComPtr<PropertyPath> spPropertyPath;

        switch (box->GetType())
        {
            case valueString:
                IFC_RETURN(UnboxValue(box, strPath.GetAddressOf()));
                IFC_RETURN(PropertyPath::CreateInstance(strPath.Get(), &spPropertyPath));
                break;

            case valueObject:
                IFC_RETURN(DXamlServices::GetPeer(box->AsObject(), &spPeer));
                IFC_RETURN(spPeer.As(&spPropertyPath));
                break;

            default:
                IFC_RETURN(E_FAIL);
                break;
        }

        IFC_RETURN(spPropertyPath.MoveTo(value));
    }
    else
    {
        *value = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ wxaml_interop::TypeName* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueTypeHandle:
                IFC_RETURN(MetadataAPI::GetTypeNameByClassInfo(MetadataAPI::GetClassInfoByIndex(box->AsTypeHandle()), value));
                break;

            case valueObject:
                {
                    CType* type = do_pointer_cast<CType>(box->AsObject());
                    IFC_RETURN(MetadataAPI::GetTypeNameByClassInfo(MetadataAPI::GetClassInfoByIndex(type->GetReferencedTypeIndex()), value));
                }
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
                break;
        }
    }
    else
    {
        value->Name = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Outptr_result_maybenull_ xaml_media::IFontFamily** value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNullOrUnset())
    {
        ctl::ComPtr<FontFamily> spFontFamily;

        switch (box->GetType())
        {
            case valueObject:
                {
                    ctl::ComPtr<DependencyObject> spPeer;
                    IFC_RETURN(DXamlServices::GetPeer(box->AsObject(), &spPeer));
                    IFC_RETURN(spPeer.As(&spFontFamily));
                }
                break;

            default:
                {
                    wrl_wrappers::HString strSource;
                    IFC_RETURN(UnboxValue(box, strSource.GetAddressOf()));
                    IFC_RETURN(ctl::make<FontFamily>(&spFontFamily));
                    IFC_RETURN(spFontFamily->put_Source(strSource.Get()));
                }
                break;
        }

        IFC_RETURN(spFontFamily.MoveTo(value));
    }
    else
    {
        *value = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml_media::Matrix* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            // TODO: Add valueMatrix type to the core
            // IFCEXPECT(box->GetType() == valueMatrix);
            // float array contains 6 elements: m11, m12, etc.
            case valueFloatArray:
                {
                    IFCEXPECT_RETURN(box->GetArrayElementCount() == 6);

                    XMATRIX* pCoreMatrix = reinterpret_cast<XMATRIX*>(box->AsFloatArray());
                    IFCPTR_RETURN(pCoreMatrix);

                    value->M11 = pCoreMatrix->m11;
                    value->M12 = pCoreMatrix->m12;
                    value->M21 = pCoreMatrix->m21;
                    value->M22 = pCoreMatrix->m22;
                    value->OffsetX = pCoreMatrix->offsetX;
                    value->OffsetY = pCoreMatrix->offsetY;
                }
                break;

            case valueObject:
                {
                    CMatrix* pMatrix = do_pointer_cast<CMatrix>(box->AsObject());
                    value->M11 = pMatrix->m_matrix.GetM11();
                    value->M12 = pMatrix->m_matrix.GetM12();
                    value->M21 = pMatrix->m_matrix.GetM21();
                    value->M22 = pMatrix->m_matrix.GetM22();
                    value->OffsetX = pMatrix->m_matrix.GetDx();
                    value->OffsetY = pMatrix->m_matrix.GetDy();
                }
                break;

            default:
                IFC_RETURN(E_FAIL);
                break;
        }
    }
    else
    {
        value->M11 = 1;
        value->M12 = 0;
        value->M21 = 0;
        value->M22 = 1;
        value->OffsetX = 0;
        value->OffsetY = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml_animation::KeyTime* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueVO:
                {
                    ASSERT(box->As<valueVO>()->GetTypeIndex() == KeyTimeVO::s_typeIndex);
                    const KeyTimeVO& vo = static_cast<const KeyTimeVO::Wrapper*>(box->As<valueVO>())->Value();
                    value->TimeSpan = TimeSpanUtil::FromSeconds(vo.GetTimeSpanInSec());
                }
                break;

            case valueObject:
                {
                    CKeyTime* keyTime = do_pointer_cast<CKeyTime>(box->AsObject());
                    value->TimeSpan = TimeSpanUtil::FromSeconds(keyTime->GetTimeSpanInSec());
                }
                break;

            default:
                IFC_RETURN(UnboxValue(box, &value->TimeSpan));
                break;
        }
    }
    else
    {
        value->TimeSpan.Duration = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml_animation::RepeatBehavior* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueDouble:
                {
                    IFC_RETURN(UnboxValue(box, &value->Duration));
                    value->Type = xaml_animation::RepeatBehaviorType_Duration;
                    value->Count = 0.0;
                }
                break;

            case valueString:
                {
                    DirectUI::RepeatBehaviorType type = DirectUI::RepeatBehaviorType::Count;
                    double durationInSec = 0.0;
                    float count = 1.0f;

                    IFC_RETURN(RepeatBehaviorFromString(
                        box->As<valueString>(),
                        &type,
                        &durationInSec,
                        &count));

                    value->Type = static_cast<xaml_animation::RepeatBehaviorType>(type);
                    value->Count = count;
                    value->Duration = TimeSpanUtil::FromSeconds(durationInSec);
                }
                break;

            case valueObject:
                {
                    CRepeatBehavior* repeatBehavior = do_pointer_cast<CRepeatBehavior>(box->AsObject());
                    value->Type = static_cast<xaml_animation::RepeatBehaviorType>(repeatBehavior->GetRepeatBehaviorType());
                    value->Count = repeatBehavior->GetCount();
                    value->Duration = TimeSpanUtil::FromSeconds(repeatBehavior->GetDurationInSec());
                }
                break;

            case valueVO:
                {
                    ASSERT(box->As<valueVO>()->GetTypeIndex() == RepeatBehaviorVO::s_typeIndex);
                    const RepeatBehaviorVO& vo = static_cast<const RepeatBehaviorVO::Wrapper*>(box->As<valueVO>())->Value();
                    value->Type = static_cast<xaml_animation::RepeatBehaviorType>(vo.GetRepeatBehaviorType());
                    value->Count = vo.GetCount();
                    value->Duration = TimeSpanUtil::FromSeconds(vo.GetDurationInSec());
                }
                break;


            default:
                IFC_RETURN(E_FAIL);
                break;
        }
    }
    else
    {
        value->Type = xaml_animation::RepeatBehaviorType_Duration;
        value->Count = 0.0;
        value->Duration.Duration = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml_media::Media3D::Matrix3D* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        XMATRIX3D* pCoreMatrix = nullptr;

        if (box->GetType() == valueObject)
        {
            CMatrix4x4* pMatrix = do_pointer_cast<CMatrix4x4>(box->AsObject());
            pCoreMatrix = reinterpret_cast<XMATRIX3D*>(&pMatrix->m_matrix);
        }
        else
        {
            unsigned arrayLength = 0;
            IFC_RETURN(box->GetFloatArray(
                reinterpret_cast<XFLOAT*&>(pCoreMatrix),
                &arrayLength));
            IFCEXPECT_RETURN(arrayLength == 16);
            IFCPTR_RETURN(pCoreMatrix);
        }

        value->M11 = pCoreMatrix->m11;
        value->M12 = pCoreMatrix->m12;
        value->M13 = pCoreMatrix->m13;
        value->M14 = pCoreMatrix->m14;
        value->M21 = pCoreMatrix->m21;
        value->M22 = pCoreMatrix->m22;
        value->M23 = pCoreMatrix->m23;
        value->M24 = pCoreMatrix->m24;
        value->M31 = pCoreMatrix->m31;
        value->M32 = pCoreMatrix->m32;
        value->M33 = pCoreMatrix->m33;
        value->M34 = pCoreMatrix->m34;
        value->OffsetX = pCoreMatrix->offsetX;
        value->OffsetY = pCoreMatrix->offsetY;
        value->OffsetZ = pCoreMatrix->offsetZ;
        value->M44 = pCoreMatrix->m44;
    }
    else
    {
        value->M11 = 0;
        value->M12 = 0;
        value->M13 = 0;
        value->M14 = 0;
        value->M21 = 0;
        value->M22 = 0;
        value->M23 = 0;
        value->M24 = 0;
        value->M31 = 0;
        value->M32 = 0;
        value->M33 = 0;
        value->M34 = 0;
        value->OffsetX = 0;
        value->OffsetY = 0;
        value->OffsetZ = 0;
        value->M44 = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _Out_ xaml_docs::TextRange* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        switch (box->GetType())
        {
            case valueTextRange:
                {
                    auto textRangeData = box->As<valueTextRange>();
                    value->StartIndex = textRangeData.startIndex;
                    value->Length = textRangeData.length;

                }
                break;

            case valueObject:
                {
                    auto textRange = do_pointer_cast<CTextRange>(box->AsObject());
                    value->StartIndex = textRange->m_range.startIndex;
                    value->Length = textRange->m_range.length;
                }
                break;
        }
    }
    else
    {
        value->StartIndex = 0;
        value->Length = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxValue(
    _In_ const CValue* box,
    _In_reads_(nFloats) XFLOAT* value,
    _In_ XUINT32 nFloats)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    if (!box->IsNull())
    {
        IFCEXPECT_RETURN(box->GetType() == valueFloatArray);
        IFCEXPECT_RETURN(box->GetArrayElementCount() >= nFloats);

        XFLOAT* floatArray = box->AsFloatArray();

        for (XUINT32 i = 0; i < nFloats; i++)
        {
            value[i] = floatArray[i];
        }
    }
    else
    {
        for (XUINT32 i = 0; i < nFloats; i++)
        {
            value[i] = 0;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxObjectValue(
    _In_ const CValue* box,
    _In_opt_ const CClassInfo* pTargetType,
    _In_ bool targetTypeIsNullable,
    _In_ REFIID riid,
    _Out_ void** value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    ctl::ComPtr<IInspectable> spInspectable;
    KnownTypeIndex hExpectedClass = KnownTypeIndex::Object;
    KnownTypeIndex hActualClass = KnownTypeIndex::Object;

    // If the value isn't null, we need to do our conversion logic
    // Otherwise, if the target type is nullable, and the value is null, we don't have to do any conversion logic
    if (!targetTypeIsNullable || !box->IsNull())
    {
        if (box->GetType() == valueIInspectable)
        {
            spInspectable = box->AsIInspectable();
        }
        else if (pTargetType != nullptr)
        {
            if (pTargetType->IsEnum() && ((box->GetType() == valueString) || (box->IsEnum())))
            {
                UINT nValue;
                IFC_RETURN(UnboxEnumValue(box, pTargetType, &nValue));
                IFC_RETURN(GetKnownWinRTBoxFromEnumValue(nValue, pTargetType, &spInspectable));
            }
            else if (!pTargetType->IsBuiltinType())
            {
                if (pTargetType->HasTypeConverter() && (box->GetType() == valueString))
                {
                    // Create the custom type from a string.
                    IFC_RETURN(ActivationAPI::ActivateInstanceFromString(pTargetType, box->AsString(), &spInspectable));
                }
                else
                {
                    const CClassInfo* pBoxedType = MetadataAPI::TryGetBoxedType(pTargetType);

                    if (pBoxedType != nullptr)
                    {
                        // For boxed values, try unboxing its boxed type instead.  E.g. for an IReference<Boolean>,
                        // we can treat it as unboxing a Boolean since it returns the desired IReference<Boolean>.
                        IFC_RETURN(UnboxObjectValue(box, pBoxedType, &spInspectable));
                    }
                }
            }
        }

        if (spInspectable == nullptr && !box->IsUnset())
        {
            CDependencyObject* pDO = box->AsObject();

            if (pDO != nullptr)
            {
                // If this is a MOR, then try to unwrap instead of doing a conversion.
                IFC_RETURN(CoreImports::DependencyObject_GetTypeIndex(pDO, &hActualClass));
            }

            if (hActualClass == KnownTypeIndex::ExternalObjectReference || hActualClass == KnownTypeIndex::ThemeResource)
            {
                hExpectedClass = hActualClass;
            }
            else if (hActualClass == KnownTypeIndex::Enumerated)
            {
                hExpectedClass = static_cast<CEnumerated*>(pDO)->GetEnumTypeIndex();
            }
            else if (box->GetType() == valueThemeResource)
            {
                hExpectedClass = KnownTypeIndex::ThemeResource;
            }
            else if (pTargetType)
            {
                hExpectedClass = pTargetType->m_nIndex;
            }

            if (hExpectedClass == KnownTypeIndex::Object)
            {
                if (pDO != nullptr)
                {
                    hExpectedClass = hActualClass;
                }
                else if (!box->IsNull())
                {
                    const CClassInfo* boxType = nullptr;
                    IFC_RETURN(GetTypeInfoFromCValue(box, &boxType));
                    if (boxType)
                    {
                        hExpectedClass = boxType->m_nIndex;
                    }

                    // FontWeights are enums internally, but publicly they're a FontWeight struct.
                    // Unfortunately, in Blue, we still sent them out as an "untyped" enum if the
                    // target type was Object. Apps took a dependency on this and expected to be
                    // able to cast the resulting object back to an Int32. If we give them a FontWeight
                    // struct, they blow up, so let's try to preserve compat for those cases.
                    if (box->IsEnum() &&
                        (hExpectedClass != KnownTypeIndex::FontWeight || !pTargetType || pTargetType->GetIndex() != KnownTypeIndex::FontWeight))
                    {
                        // All boxed enums should target an actual type
                        ASSERT(boxType);

                        UINT nValue;
                        IFC_RETURN(UnboxEnumValue(box, boxType, &nValue));

                        if (boxType->GetIndex() == KnownTypeIndex::Int32 || boxType->GetIndex() == KnownTypeIndex::FontWeight)
                        {
                            IFC_RETURN(PropertyValue::CreateFromInt32(nValue, &spInspectable));
                        }
                        else
                        {
                            IFC_RETURN(GetKnownWinRTBoxFromEnumValue(nValue, boxType, &spInspectable));
                        }
                    }
                    else if (box->GetType() == valueSignedArray)
                    {
                        wfc::IVectorView<INT>* pIVV = nullptr;
                        ctl::ComPtr<ValueTypeView<INT>> spSignedVTV;
                        IFC_RETURN(ctl::make(&spSignedVTV));
                        IFC_RETURN(spSignedVTV->SetView(box->AsSignedArray(), box->GetArrayElementCount()));
                        IFC_RETURN(spSignedVTV.MoveTo(&pIVV));
                        spInspectable.Attach(ctl::as_iinspectable(pIVV));
                    }
                    else if (box->GetType() == valueDoubleArray)
                    {
                        wfc::IVectorView<DOUBLE>* pIVV = nullptr;
                        ctl::ComPtr<ValueTypeView<DOUBLE>> spDoubleVTV;
                        IFC_RETURN(ctl::make(&spDoubleVTV));
                        IFC_RETURN(spDoubleVTV->SetView(box->AsDoubleArray(), box->GetArrayElementCount()));
                        IFC_RETURN(spDoubleVTV.MoveTo(&pIVV));
                        spInspectable.Attach(ctl::as_iinspectable(pIVV));
                    }
                }
            }

            if (spInspectable == nullptr)
            {
                switch (hExpectedClass)
                {
                    case KnownTypeIndex::TypeName:
                        {
                            wxaml_interop::TypeName rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<wxaml_interop::TypeName>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Boolean:
                        {
                            BOOLEAN rawValue = 0;
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromBoolean(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Int32:
                        {
                            INT rawValue = 0;
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromInt32(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Int64:
                        {
                            INT64 rawValue = 0;
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromInt64(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Float:
                        {
                            FLOAT rawValue = 0;
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromSingle(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Double:
                        {
                            DOUBLE rawValue = 0;
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromDouble(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::String:
                        {
                            if (!box->IsNullOrUnset() || box->GetType() == valueString)
                            {
                                wrl_wrappers::HString rawValue;
                                IFC_RETURN(UnboxValue(box, rawValue.GetAddressOf()));
                                IFC_RETURN(PropertyValue::CreateFromString(rawValue.Get(), &spInspectable));
                            }
                        }
                        break;

                    case KnownTypeIndex::Duration:
                        {
                            xaml::Duration rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<xaml::Duration>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::FontWeight:
                        {
                            wut::FontWeight rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<wut::FontWeight>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::CornerRadius:
                        {
                            xaml::CornerRadius rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<xaml::CornerRadius>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Thickness:
                        {
                            xaml::Thickness rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<xaml::Thickness>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::GridLength:
                        {
                            xaml::GridLength rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<xaml::GridLength>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::KeyTime:
                        {
                            xaml_animation::KeyTime rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<xaml_animation::KeyTime>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::RepeatBehavior:
                        {
                            xaml_animation::RepeatBehavior rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<xaml_animation::RepeatBehavior>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Color:
                        {
                            wu::Color rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<wu::Color>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Matrix:
                        {
                            xaml_media::Matrix rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<xaml_media::Matrix>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Matrix3D:
                        {
                            xaml_media::Media3D::Matrix3D rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateReference<xaml_media::Media3D::Matrix3D>(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Size:
                        {
                            wf::Size rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromSize(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Point:
                        {
                            wf::Point rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromPoint(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::Rect:
                        {
                            wf::Rect rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromRect(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::TimeSpan:
                        {
                            wf::TimeSpan rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromTimeSpan(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::DateTime:
                        {
                            wf::DateTime rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromDateTime(rawValue, &spInspectable));
                        }
                        break;

                    case KnownTypeIndex::FontFamily:
                        {
                            IFontFamily* pResult = nullptr;
                            IFC_RETURN(UnboxValue(box, &pResult));
                            spInspectable.Attach(pResult);
                        }
                        break;

                    case KnownTypeIndex::Uri:
                        {
                            wf::IUriRuntimeClass* pResult = nullptr;
                            IFC_RETURN(UnboxValue(box, &pResult));
                            spInspectable.Attach(pResult);
                        }
                        break;

                    case KnownTypeIndex::PropertyPath:
                        {
                            xaml::IPropertyPath* pResult = nullptr;
                            IFC_RETURN(UnboxValue(box, &pResult));
                            spInspectable.Attach(pResult);
                        }
                        break;

                    case KnownTypeIndex::ThemeResource:
                        {
                            ThemeResourceExpression* pResult = nullptr;

                            if (pDO)
                            {
                                xref_ptr<CThemeResource> pThemeResource = make_xref<CThemeResource>(static_cast<CThemeResourceExtension*>(pDO));
                                IFC_RETURN(ThemeResourceExpression::Create(pThemeResource, &pResult));
                            }
                            else
                            {
                                IFC_RETURN(ThemeResourceExpression::Create(box->AsThemeResource(), &pResult));
                            }

                            spInspectable.Attach(ctl::as_iinspectable(pResult));
                        }
                        break;

                    case KnownTypeIndex::TextRange:
                        {
                            xaml_docs::TextRange rawValue = {};
                            IFC_RETURN(UnboxValue(box, &rawValue));
                            IFC_RETURN(PropertyValue::CreateFromTextRange(rawValue, &spInspectable));
                        }
                        break;

                    default:
                        if (pDO != nullptr)
                        {
                            if (pDO->OfTypeByIndex<KnownTypeIndex::Enumerated>())
                            {
                                CEnumerated* pEnum = static_cast<CEnumerated*>(pDO);
                                IFC_RETURN(GetKnownWinRTBoxFromEnumValue(pEnum->m_nValue, MetadataAPI::GetClassInfoByIndex(pEnum->GetEnumTypeIndex()), &spInspectable));
                            }
                            else
                            {
                                // Marshal as generic object.
                                ctl::ComPtr<DependencyObject> spWrapper;
                                IFC_RETURN(DXamlServices::GetPeer(pDO, &spWrapper));
                                UnwrapExternalObjectReferenceIfPresent(ctl::iinspectable_cast(spWrapper.Get()), &spInspectable);
                            }
                        }
                        break;
                }
            }
        }
    }

    if (spInspectable != nullptr)
    {
        IFC_RETURN(spInspectable.Get()->QueryInterface(riid, value));
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnboxObjectValue(
            _In_ const CValue* box,
            _In_opt_ const CClassInfo* pTargetType,
            _In_ REFIID riid,
            _Out_ void** result)
{
     return UnboxObjectValue(
         box,
         pTargetType,
         /* targetTypeIsNullable */ pTargetType != nullptr && MetadataAPI::RepresentsBoxedType(pTargetType),
         riid,
         result);
}

_Check_return_ HRESULT CValueBoxer::UnboxObjectValue(
    _In_ const CValue* box,
    _In_opt_ const CClassInfo* pTargetType,
    _Out_ IInspectable** value
)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    IFC_RETURN(UnboxObjectValue(box, pTargetType, __uuidof(IInspectable), reinterpret_cast<void**>(value)));

    return S_OK;
}

void CValueBoxer::UnwrapExternalObjectReferenceIfPresent(
    _In_ IInspectable* pObject,
    _Outptr_ IInspectable** value)
{
    ctl::ComPtr<ExternalObjectReference> spObj = ctl::query_interface_cast<ExternalObjectReference>(pObject);

    if (spObj)
    {
        spObj->get_Target(value);
    }
    else
    {
        *value = pObject;
        AddRefInterface(pObject);
    }
}

_Check_return_ HRESULT CValueBoxer::GetTypeInfoFromCValue(
    _In_ const CValue* box,
    _Out_ const CClassInfo** ppTypeInfo)
{
    switch (box->GetType())
    {
        case valueEnum:
        case valueEnum8:
            {
                uint32_t enumValue = 0;
                KnownTypeIndex enumTypeIndex = KnownTypeIndex::UnknownType;

                box->GetEnum(enumValue, enumTypeIndex);

                if (enumTypeIndex != KnownTypeIndex::UnknownType)
                {
                    *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(enumTypeIndex);
                }
                else
                {
                    // If we can't determine the enum type, assume Int32.
                    *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Int32);
                }
            }
            break;

        case valueColor:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Color);
            break;

        case valueThickness:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Thickness);
            break;

        case valueCornerRadius:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::CornerRadius);
            break;

        case valueGridLength:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::GridLength);
            break;

        case valueBool:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Boolean);
            break;

        case valueSigned:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Int32);
            break;

        case valueInt64:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Int64);
            break;

        case valueFloat:
        case valueDouble:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Double);
            break;

        case valueString:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::String);
            break;

        case valuePoint:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Point);
            break;

        case valueSize:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Size);
            break;

        case valueRect:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Rect);
            break;

        case valueTimeSpan:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TimeSpan);
            break;

        case valueDateTime:
            *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DateTime);
            break;

        case valueFloatArray:
            if (box->GetArrayElementCount() == 6)
            {
                *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Matrix);
            }
            else if (box->GetArrayElementCount() == 16)
            {
                *ppTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Matrix3D);
            }
            else
            {
                // Unknown type.
                IFC_RETURN(E_FAIL);
            }
            break;

        default:
            // Not yet supported
            *ppTypeInfo = nullptr;
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::ResolveEnumValueFromString(
    _In_reads_(nLength) const WCHAR* pszValue,
    _In_ XUINT32 nLength,
    _In_ const CClassInfo* pSourceType,
    _Out_ UINT* value)
{
    IFCEXPECT_RETURN(pSourceType->HasTypeConverter() && pSourceType->IsEnum());

    UINT nValueTableLength = 0;
    const XTABLE* aValueTable = nullptr;
    INT nValue = 0;

    IFC_RETURN(GetEnumValueTable(pSourceType->GetIndex(), &nValueTableLength, &aValueTable));
    IFC_RETURN(FlagsEnumerateFromString(nValueTableLength, aValueTable, nLength, pszValue, &nValue));
    *value = static_cast<UINT>(nValue);

    return S_OK;
}

_Check_return_ HRESULT CValueBoxer::UnwrapWeakRef(
    _In_ const CValue* const value,
    _In_ const CDependencyProperty* dp,
    _Outptr_result_maybenull_ CDependencyObject** unwrappedElement)
{
    *unwrappedElement = nullptr;

    ctl::ComPtr<IInspectable> spValue;
    IFC_RETURN(CValueBoxer::UnboxObjectValue(value, dp->GetPropertyType(), &spValue));

    if (spValue)
    {
        ctl::ComPtr<IDependencyObject> spObject;
        spObject.Attach(ValueWeakReference::get_value_as<IDependencyObject>(spValue.Get()));

        if (spObject)
        {
            *unwrappedElement = spObject.Cast<DependencyObject>()->GetHandle();
        }
    }

    return S_OK;
}
