// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CValueConvert.h>
#include <CValue.h>
#include <AutomationCValue.h>
#include <Rect.h>
#include <Size.h>
#include <Point.h>
#include <Thickness.h>
#include <CornerRadius.h>
#include <GridLength.h>
#include <CMatrix.h>
#include <KeyTime.h>
#include <Duration.h>
#include <RepeatBehavior.h>
#include <Primitives.h>
#include <Type.h>
#include <xstrutil.h>
#include <CDependencyObject.h>
#include <TypeTableStructs.h>
#include <DOPointerCast.h>
#include <TimeSpan.h>
#include <StringConversions.h>

namespace CValueConvert
{
    template _Check_return_ HRESULT TryUnboxPropertyValue(
        _In_ wf::IPropertyValue* inValue,
        _Out_ ::CValue& outValue,
        _Out_ bool& success);

    template _Check_return_ HRESULT EnsurePropertyValueUnboxed(
        _In_ const ::CValue& inValue,
        _Out_ ::CValue& outValue);

    template _Check_return_ HRESULT TryUnboxCDependencyObjectValue(
        _In_ CDependencyObject* inValue,
        _Out_ ::CValue& outValue,
        _Out_ bool& success);

    template _Check_return_ HRESULT EnsureCDependencyObjectValueUnboxed(
        _In_ const ::CValue& inValue,
        _Out_ ::CValue& outValue);

    template _Check_return_ HRESULT TryUnboxValueObject(
        _In_ const Flyweight::PropertyValueObjectBase* inValue,
        _Out_ ::CValue& outValue,
        _Out_ bool& success);

    template _Check_return_ HRESULT EnsureValueObjectUnboxed(
        _In_ const ::CValue& inValue,
        _Out_ ::CValue& outValue);

    template _Check_return_ HRESULT TryUnboxPropertyValue(
        _In_ wf::IPropertyValue* inValue,
        _Out_ Automation::CValue& outValue,
        _Out_ bool& success);

    template _Check_return_ HRESULT EnsurePropertyValueUnboxed(
        _In_ const Automation::CValue& inValue,
        _Out_ Automation::CValue& outValue);

    template _Check_return_ HRESULT TryUnboxCDependencyObjectValue(
        _In_ CDependencyObject* inValue,
        _Out_ Automation::CValue& outValue,
        _Out_ bool& success);

    template _Check_return_ HRESULT EnsureCDependencyObjectValueUnboxed(
        _In_ const Automation::CValue& inValue,
        _Out_ Automation::CValue& outValue);

    template<typename CValueType>
    _Check_return_ HRESULT TryUnboxPropertyValue(
        _In_ wf::IPropertyValue* inValue,
        _Out_ CValueType& outValue,
        _Out_ bool& success)
    {
        // Quick and dirty way to convert the most common types to a CValue.

        ASSERT(inValue);

        success = false;

        wf::PropertyType ePropertyType = wf::PropertyType::PropertyType_Empty;
        IFC_RETURN(inValue->get_Type(&ePropertyType));

        switch (ePropertyType)
        {
            case wf::PropertyType_Boolean:
                {
                    BOOLEAN bValue = FALSE;
                    IFC_RETURN(inValue->GetBoolean(&bValue));
                    outValue.SetBool(!!bValue);
                }
                break;

            case wf::PropertyType_Int32:
                {
                    INT32 nValue = 0;
                    IFC_RETURN(inValue->GetInt32(&nValue));
                    outValue.SetSigned(nValue);
                }
                break;

            case wf::PropertyType_UInt32:
                {
                    // TODO: We need to introduce valueUnsigned for correctness.
                    UINT32 nValue = 0;
                    IFC_RETURN(inValue->GetUInt32(&nValue));
                    outValue.SetSigned((INT32)nValue);
                }
                break;

            case wf::PropertyType_Int64:
                {
                    INT64 value = 0;
                    IFC_RETURN(inValue->GetInt64(&value));
                    outValue.SetInt64(value);
                }
                break;

            case wf::PropertyType_UInt64:
                {
                    UINT64 value = 0;
                    IFC_RETURN(inValue->GetUInt64(&value));
                    outValue.SetInt64(static_cast<INT64>(value));
                }
                break;

            case wf::PropertyType_Double:
                {
                    DOUBLE nValue = 0.0;
                    IFC_RETURN(inValue->GetDouble(&nValue));
                    outValue.SetDouble(nValue);
                }
                break;

            case wf::PropertyType_Single:
                {
                    FLOAT nValue = 0.0f;
                    IFC_RETURN(inValue->GetSingle(&nValue));
                    outValue.SetFloat(nValue);
                }
                break;

            case wf::PropertyType_String:
                {
                    wrl_wrappers::HString str;
                    IFC_RETURN(inValue->GetString(str.GetAddressOf()));
                    IFC_RETURN(outValue.SetString(str.Get()));
                }
                break;

            case wf::PropertyType_TimeSpan:
                {
                    wf::TimeSpan timeSpan = {};
                    IFC_RETURN(inValue->GetTimeSpan(&timeSpan));
                    outValue.SetTimeSpan(timeSpan);
                }
                break;

            case wf::PropertyType_DateTime:
                {
                    wf::DateTime dateTime = {};
                    IFC_RETURN(inValue->GetDateTime(&dateTime));
                    outValue.SetDateTime(dateTime);
                }
                break;

            case wf::PropertyType_OtherType:
                // We don't have a good way to compare custom structs.
                // early exit without setting success to true
                return S_OK;

            default:
                // NOTE: If there are other commonly used types, consider supporting those here for a small CPU improvement
                //       when doing a check to see if a property value changed.
                // early exit without setting success to true
                return S_OK;
        }

        success = true;

        return S_OK;
    }

    template<typename CValueType>
    _Check_return_ HRESULT EnsurePropertyValueUnboxed(
        _In_ const CValueType& inValue,
        _Out_ CValueType& outValue)
    {
        ctl::ComPtr<IInspectable> inValueAsIInspectable = inValue.AsIInspectable();
        CValueType result;
        bool unboxed = false;

        if (inValueAsIInspectable)
        {
            ctl::ComPtr<wf::IPropertyValue> inValueAsPV = inValueAsIInspectable.AsOrNull<wf::IPropertyValue>();

            if (inValueAsPV)
            {
                IFC_RETURN(CValueConvert::TryUnboxPropertyValue(
                    inValueAsPV.Get(),
                    result,
                    unboxed));
            }
        }

        if (!unboxed)
        {
            result.WrapValue(inValue);
        }

        outValue = std::move(result);

        return S_OK;
    }

    static _Check_return_ HRESULT GetContentPropertyValueHelper(
        _In_ CDependencyObject* inValue,
        _Out_ CValue& outValue)
    {
        const CDependencyProperty* prop = inValue->GetContentProperty();
        IFCEXPECT_RETURN(prop);
        IFC_RETURN(inValue->GetValue(prop, &outValue));
        return S_OK;
    }

    template<typename CValueType>
    _Check_return_ HRESULT TryUnboxCDependencyObjectValue(
        _In_ CDependencyObject* inValue,
        _Out_ CValueType& outValue,
        _Out_ bool& success)
    {
        ASSERT(inValue);

        success = false;

        switch (inValue->GetTypeIndex())
        {
            // TODO: scalar types + string can be changed to: (once automation::cvalue is removed)
            //const CDependencyProperty* prop = inValue->GetContentProperty();
            //IFCEXPECT_RETURN(prop);
            //IFC_RETURN(inValue->GetValue(prop, &temp));
            //outValue = std::move(temp);

            case KnownTypeIndex::Int32:
                {
                    CValue temp;
                    IFC_RETURN(GetContentPropertyValueHelper(inValue, temp));
                    ASSERT(temp.GetType() == valueSigned);
                    outValue.SetSigned(temp.AsSigned());
                }
                break;

            case KnownTypeIndex::Int64:
                {
                    CValue temp;
                    IFC_RETURN(GetContentPropertyValueHelper(inValue, temp));
                    ASSERT(temp.GetType() == valueInt64);
                    outValue.SetInt64(temp.AsInt64());
                }
                break;

            case KnownTypeIndex::Double:
                {
                    CValue temp;
                    IFC_RETURN(GetContentPropertyValueHelper(inValue, temp));
                    ASSERT(temp.GetType() == valueFloat);
                    outValue.SetFloat(temp.AsFloat());
                }
                break;

            case KnownTypeIndex::String:
                {
                    CValue temp;
                    IFC_RETURN(GetContentPropertyValueHelper(inValue, temp));
                    ASSERT(temp.GetType() == valueString);
                    outValue.SetString(temp.AsString());
                }
                break;

            case KnownTypeIndex::Color:
                {
                    CValue temp;
                    IFC_RETURN(GetContentPropertyValueHelper(inValue, temp));
                    ASSERT(temp.GetType() == valueColor);
                    outValue.SetColor(temp.AsColor());
                }
                break;

            case KnownTypeIndex::Rect:
                outValue.SetRect(new XRECTF(checked_cast<CRect>(inValue)->m_rc));
                break;

            case KnownTypeIndex::Size:
                outValue.SetSize(new XSIZEF(checked_cast<CSize>(inValue)->m_size));
                break;

            case KnownTypeIndex::Matrix:
                {
                    CMatrix* sourceDO = checked_cast<CMatrix>(inValue);
                    XFLOAT* newValue = new float[6];
                    newValue[0] = sourceDO->m_matrix.GetM11();
                    newValue[1] = sourceDO->m_matrix.GetM12();
                    newValue[2] = sourceDO->m_matrix.GetM21();
                    newValue[3] = sourceDO->m_matrix.GetM22();
                    newValue[4] = sourceDO->m_matrix.GetDx();
                    newValue[5] = sourceDO->m_matrix.GetDy();
                    outValue.SetFloatArray(6, newValue);
                }
                break;

            case KnownTypeIndex::Matrix3D:
                {
                    CMatrix4x4* sourceDO = checked_cast<CMatrix4x4>(inValue);
                    XFLOAT* newValue = new float[16];
                    newValue[0] = sourceDO->m_matrix.GetM11();
                    newValue[1] = sourceDO->m_matrix.GetM12();
                    newValue[2] = sourceDO->m_matrix.GetM13();
                    newValue[3] = sourceDO->m_matrix.GetM14();
                    newValue[4] = sourceDO->m_matrix.GetM21();
                    newValue[5] = sourceDO->m_matrix.GetM22();
                    newValue[6] = sourceDO->m_matrix.GetM23();
                    newValue[7] = sourceDO->m_matrix.GetM24();
                    newValue[8] = sourceDO->m_matrix.GetM31();
                    newValue[9] = sourceDO->m_matrix.GetM32();
                    newValue[10] = sourceDO->m_matrix.GetM33();
                    newValue[11] = sourceDO->m_matrix.GetM34();
                    newValue[12] = sourceDO->m_matrix.GetM41();
                    newValue[13] = sourceDO->m_matrix.GetM42();
                    newValue[14] = sourceDO->m_matrix.GetM43();
                    newValue[15] = sourceDO->m_matrix.GetM44();
                    outValue.SetFloatArray(16, newValue);
                }
                break;

            case KnownTypeIndex::KeyTime:
                {
                    auto wrapper = checked_cast<CKeyTime>(inValue)->ValueWrapper();

                    if (wrapper)
                    {
                        IFC_RETURN(TryUnboxValueObject(
                            wrapper,
                            outValue,
                            success));
                    }

                    // Exit early not to set the success result.
                    return S_OK;
                }
                break;

            case KnownTypeIndex::Duration:
                {
                    auto wrapper = checked_cast<CDuration>(inValue)->ValueWrapper();

                    if (wrapper)
                    {
                        IFC_RETURN(TryUnboxValueObject(
                            wrapper,
                            outValue,
                            success));
                    }

                    // Exit early not to set the success result.
                    return S_OK;
                }
                break;

            case KnownTypeIndex::RepeatBehavior:
                {
                    auto wrapper = checked_cast<CRepeatBehavior>(inValue)->ValueWrapper();

                    if (wrapper)
                    {
                        IFC_RETURN(TryUnboxValueObject(
                            wrapper,
                            outValue,
                            success));
                    }

                    // Exit early not to set the success result.
                    return S_OK;
                }
                break;

            case KnownTypeIndex::TimeSpan:
                outValue.SetDouble(checked_cast<CTimeSpan>(inValue)->m_rTimeSpan);
                break;

            case KnownTypeIndex::Point:
                outValue.SetPoint(new XPOINTF(checked_cast<CPoint>(inValue)->m_pt));
                break;

            case KnownTypeIndex::Thickness:
                outValue.SetThickness(new XTHICKNESS(checked_cast<CThickness>(inValue)->m_thickness));
                break;

            case KnownTypeIndex::CornerRadius:
                outValue.SetCornerRadius(new XCORNERRADIUS(checked_cast<CCornerRadius>(inValue)->m_cornerRadius));
                break;

            case KnownTypeIndex::GridLength:
                outValue.SetGridLength(new XGRIDLENGTH(checked_cast<CGridLength>(inValue)->m_gridLength));
                break;

            case KnownTypeIndex::TypeName:
                outValue.SetTypeHandle(checked_cast<CType>(inValue)->GetReferencedTypeIndex());
                break;

            default:
                if (inValue->GetClassInformation()->IsEnum())
                {
                    CEnumerated* enumDO = checked_cast<CEnumerated>(inValue);

                    if (enumDO->GetEnumTypeIndex() == KnownTypeIndex::Boolean)
                    {
                        // Enum of sub-type INDEX_BOOLEAN has a separate ValueType,
                        // valueBool.  Use it.
                        outValue.SetBool(!!enumDO->m_nValue);
                    }
                    else
                    {
                        outValue.SetEnum(enumDO->m_nValue, enumDO->GetEnumTypeIndex());
                    }
                }
                else
                {
                    // early exit without setting success to true
                    return S_OK;
                }
                break;
        }

        success = true;

        return S_OK;
    }

    template<typename CValueType>
    _Check_return_ HRESULT EnsureCDependencyObjectValueUnboxed(
        _In_ const CValueType& inValue,
        _Out_ CValueType& outValue)
    {
        CDependencyObject* inValueAsDO = inValue.AsObject();
        CValueType result;
        bool unboxed = false;

        if (inValueAsDO)
        {
            IFC_RETURN(TryUnboxCDependencyObjectValue(
                inValueAsDO,
                result,
                unboxed));
        }

        if (!unboxed)
        {
            result.SetObjectAddRef(inValueAsDO);
        }

        outValue = std::move(result);

        return S_OK;
    }

    template<typename CValueType>
    _Check_return_ HRESULT TryUnboxValueObject(
        _In_ const Flyweight::PropertyValueObjectBase* inValue,
        _Out_ CValueType& outValue,
        _Out_ bool& success)
    {
        ASSERT(inValue);

        success = false;

        switch (inValue->GetTypeIndex())
        {
            case KnownTypeIndex::KeyTime:
                {
                    const KeyTimeVO& value = static_cast<const KeyTimeVO::Wrapper*>(inValue)->Value();
                    outValue.SetDouble(value.GetTimeSpanInSec());
                }
                break;

            case KnownTypeIndex::Duration:
                {
                    const DurationVO& value = static_cast<const DurationVO::Wrapper*>(inValue)->Value();

                    switch (value.GetDurationType())
                    {
                        case DirectUI::DurationType::Automatic:
                        case DirectUI::DurationType::Forever:
                            {
                                xstring_ptr strValue;

                                IFC_RETURN(DurationToString(
                                    value.GetDurationType(),
                                    value.GetTimeSpanInSec(),
                                    strValue));

                                outValue.SetString(std::move(strValue));
                            }
                            break;

                        case DirectUI::DurationType::TimeSpan:
                            outValue.SetDouble(value.GetTimeSpanInSec());
                            break;

                        default:
                            IFC_RETURN(E_UNEXPECTED);
                            break;
                    }
                }
                break;

            case KnownTypeIndex::RepeatBehavior:
                {
                    const RepeatBehaviorVO& value = static_cast<const RepeatBehaviorVO::Wrapper*>(inValue)->Value();

                    switch (value.GetRepeatBehaviorType())
                    {
                        case DirectUI::RepeatBehaviorType::Count:
                        case DirectUI::RepeatBehaviorType::Forever:
                            {
                                xstring_ptr strValue;

                                IFC_RETURN(RepeatBehaviorToString(
                                    value.GetRepeatBehaviorType(),
                                    value.GetDurationInSec(),
                                    value.GetCount(),
                                    strValue));

                                outValue.SetString(std::move(strValue));
                            }
                            break;

                        case DirectUI::RepeatBehaviorType::Duration:
                            outValue.SetDouble(value.GetDurationInSec());
                            break;

                        default:
                            IFC_RETURN(E_UNEXPECTED);
                            break;
                    }
                }
                break;

            default:
                // early exit without setting success to true
                return S_OK;
        }

        success = true;

        return S_OK;
    }

    template<typename CValueType>
    _Check_return_ HRESULT EnsureValueObjectUnboxed(
        _In_ const CValueType& inValue,
        _Out_ CValueType& outValue)
    {
        const Flyweight::PropertyValueObjectBase* inValueAsVO = inValue.template As<valueVO>();
        CValueType result;
        bool unboxed = false;

        if (inValueAsVO)
        {
            IFC_RETURN(TryUnboxValueObject(
                inValueAsVO,
                result,
                unboxed));
        }

        if (!unboxed)
        {
            result.template SetAddRef<valueVO>(const_cast<Flyweight::PropertyValueObjectBase*>(inValueAsVO));
        }

        outValue = std::move(result);

        return S_OK;
    }
}