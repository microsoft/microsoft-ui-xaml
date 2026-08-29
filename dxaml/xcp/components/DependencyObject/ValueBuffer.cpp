// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <MetadataAPI.h>
#include <CDependencyObject.h>
#include <UIElement.h>
#include <dopointercast.h>
#include <TypeTableStructs.h>
#include <GridLength.h>
#include <UIAEnums.h>
#include <EnumDefs.h>
#include <primitives.h>
#include <CString.h>
#include <Point.h>
#include <TextRange.h>
#include <Rect.h>
#include <Size.h>
#include <Double.h>
#include <DoubleUtil.h>
#include <InheritedProperties.h>
#include <DOCollection.h>
#include <Type.h>
#include <ModifiedValue.h>
#include <ThemeResourceExtension.h>
#include <framework.h>
#include <ValueBuffer.h>
#include "Thickness.h"
#include "Duration.h"
#include "KeyTime.h"
#include "RepeatBehavior.h"
#include "CornerRadius.h"
#include "CColor.h"
#include "SolidColorBrush.h"
#include <StringConversions.h>
#include <Clock.h>

using namespace DirectUI;

// Repackage the value so the types match.
_Check_return_ HRESULT ValueBuffer::Repackage(_Const_ _In_ const CDependencyProperty* pDP)
{
    return RepackageValue(pDP, &m_value);
}

// Repackage the incoming value so the types match, and set the given value pointer.
_Check_return_ HRESULT ValueBuffer::RepackageValueAndSetPtr(_Const_ _In_ const CDependencyProperty* pDP, _Const_ _In_ const CValue* value, _Inout_ CValue** ppValue)
{
    m_value.Unset();

    IFC_RETURN(RepackageValue(pDP, value));

    if (!m_value.IsUnset())
    {
        *ppValue = &m_value;
    }

    return S_OK;
}

// Repackage the incoming value so the types match, and copy it to the given value object.
_Check_return_ HRESULT ValueBuffer::RepackageValueAndCopy(_Const_ _In_ const CDependencyProperty* pDP, _Const_ _In_ const CValue* value, _Inout_ CValue& outValue)
{
    m_value.Unset();

    IFC_RETURN(RepackageValue(pDP, value));

    if (!m_value.IsUnset())
    {
        IFC_RETURN(outValue.CopyConverted(m_value));
    }

    return S_OK;
}

// Repackage the incoming value so the types match.
_Check_return_ HRESULT ValueBuffer::RepackageValue(_Const_ _In_ const CDependencyProperty* pDP, _Const_ _In_ const CValue* value)
{
    ValueType storageType = pDP->GetStorageType();
    CValue* pValue = const_cast<CValue*>(value);

    // If it's a DependencyObject try to convert it to the appropriate CValue so that the type converters work
    if (pValue->AsObject() != nullptr)
    {
        // this handles types that are likely to need conversion, but ultimately there should be a
        // more deterministic way to prepare a "boxed" value for type conversion.
        CDependencyObject* valueAsDO = pValue->AsObject();

        switch (valueAsDO->GetTypeIndex())
        {
            case KnownTypeIndex::Int32:
                m_value.SetSigned((static_cast<CInt32*>(valueAsDO)->m_iValue));
                pValue = &m_value;
                break;

            case KnownTypeIndex::String:
                m_value.SetString((static_cast<CString*>(valueAsDO))->m_strString);
                pValue = &m_value;
                break;
            case KnownTypeIndex::Color:
                m_value.SetColor(checked_cast<CColor>(valueAsDO)->m_rgb);
                pValue = &m_value;
                break;
        }
    }

    ValueType valueType = pValue->GetType();

    if (storageType != valueType  &&
        storageType != valueAny && // A property registered as "valueAny" signifies that it can take any CValue and would not require conversion.
        !(storageType == valueObject && (valueType == valueNull || valueType == valueIInspectable)))
    {
        switch (valueType)
        {
            case valueString:
                IFC_RETURN(ValueBuffer::RepackageValue_String(pDP, storageType, pValue));
                break;

            case valueObject:
                IFC_RETURN(ValueBuffer::RepackageValue_Object(pDP, storageType, pValue, true));
                break;

            case valueFloat:
                IFC_RETURN(ValueBuffer::RepackageValue_Float(pDP, storageType, pValue));
                break;

            case valueSigned:
                IFC_RETURN(ValueBuffer::RepackageValue_Int(pDP, storageType, pValue));
                break;

            case valueEnum:
            case valueEnum8:
                IFC_RETURN(ValueBuffer::RepackageValue_Enum(pDP, storageType, pValue));
                break;

            case valueBool:
                IFC_RETURN(ValueBuffer::RepackageValue_Bool(pDP, storageType, pValue));
                break;

            case valueSize:
                IFC_RETURN(ValueBuffer::RepackageValue_Size(pDP, storageType, pValue));
                break;

            case valuePoint:
                IFC_RETURN(ValueBuffer::RepackageValue_Point(pDP, storageType, pValue));
                break;

            case valueTextRange:
                IFC_RETURN(ValueBuffer::RepackageValue_TextRange(pDP, storageType, pValue));
                break;

            case valueNull:
                IFC_RETURN(ValueBuffer::RepackageValue_Null(pDP, storageType, pValue));
                break;

            case valueColor:
            case valuePointArray:
            case valueFloatArray:
                IFC_RETURN(ValueBuffer::RepackageValue_DynamicToObject(pDP, storageType, pValue));
                break;

            case valueDouble:
                IFC_RETURN(ValueBuffer::RepackageValue_Double(pDP, storageType, pValue));
                break;

            case valueTimeSpan:
                IFC_RETURN(ValueBuffer::RepackageValue_TimeSpan(pDP, storageType, pValue));
                break;

            case valueThickness:
                IFC_RETURN(ValueBuffer::RepackageValue_Thickness(pDP, storageType, pValue));
                break;

            case valueAny:
            default:
                IFC_RETURN(E_INVALIDARG);
        }

        ASSERT(m_value.GetType() == storageType ||
               (pDP->IsNullable() && valueType == valueNull && m_value.GetType()));
    }

    return S_OK;
}

template <typename T>
static _Check_return_ HRESULT GetValueFromCDouble(
    _In_ const CDependencyObject* valueAsObject,
    _Out_ T& result)
{
    // This is really wrong, but we need to keep it for compat reasons.
    if (valueAsObject)
    {
        const float* value = &(reinterpret_cast<const CDouble*>(valueAsObject)->m_eValue);
        result = *reinterpret_cast<const T*>(value);
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

// Version for intrinsic types
template <typename TDO, typename T>
static _Check_return_ HRESULT GetValueHelper(
    _In_ const CDependencyObject* valueAsObject,
    _In_ const T TDO::*memberVariable,
    _Out_ T& result)
{
    return GetValueFromCDouble<T>(
        valueAsObject,
        result);
}

// Version for float array kinds
template <typename TDO, typename T>
static _Check_return_ HRESULT GetValueHelper(
    _In_ const CDependencyObject* valueAsObject,
    _In_ const T TDO::*memberVariable,
    KnownTypeIndex typeIndex,
    _Out_ const T*& result)
{
    // This is really wrong, but we need to keep it for compat reasons.
    if (valueAsObject->OfTypeByIndex(typeIndex))
    {
        const XRECTF* value = &(reinterpret_cast<const CRect*>(valueAsObject)->m_rc);
        result = reinterpret_cast<const T*>(value);
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Object(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue,
    bool failOnUnhandled)
{
    ASSERT(pValue->GetType() == valueObject);

    CDependencyObject* incomingValue = pValue->AsObject();

    switch (storageType)
    {
        case valueFloat:
            {
                float floatValue = 0.0f;

                IFC_RETURN(GetValueHelper(
                    incomingValue,
                    &CDouble::m_eValue,
                    floatValue));

                m_value.SetFloat(floatValue);
            }
            break;

        case valueDouble:
            {
                IFCCHECK_RETURN(incomingValue->OfTypeByIndex<KnownTypeIndex::Double>());

                float floatValue = 0.0;

                IFC_RETURN(GetValueHelper(
                    incomingValue,
                    &CDouble::m_eValue,
                    floatValue));

                m_value.SetDouble(floatValue);
            }
            break;

        case valueSigned:
            {
                int intValue = 0;

                IFC_RETURN(GetValueHelper(
                    incomingValue,
                    &CInt32::m_iValue,
                    intValue));

                m_value.SetSigned(intValue);
            }
            break;

        case valueEnum:
        case valueEnum8:
            {
                XINT32 enumValue = 0;

                HRESULT hr = GetValueHelper(
                    incomingValue,
                    &CEnumerated::m_nValue,
                    enumValue);

                if (!SUCCEEDED(hr))
                {
                    if (hr == E_INVALIDARG)
                    {
                        IFC_RETURN(GetValueFromCDouble(
                            incomingValue,
                            enumValue));
                    }
                    else
                    {
                        return hr;
                    }
                }

                if (storageType == valueEnum)
                {
                    m_value.SetEnum(enumValue);
                }
                else
                {
                    ASSERT(enumValue <= UINT8_MAX);
                    m_value.SetEnum8(static_cast<uint8_t>(enumValue));
                }
            }
            break;

        case valueBool:
            {
                XINT32 boolValue = 0;

                IFC_RETURN(GetValueHelper(
                    incomingValue,
                    &CEnumerated::m_nValue,
                    boolValue));

                m_value.SetBool(!!boolValue);
            }
            break;

        case valueColor:
            {
                UINT32 rgbValue = 0;

                HRESULT hr = GetValueHelper(
                    incomingValue,
                    &CColor::m_rgb,
                    rgbValue);

                if (!SUCCEEDED(hr))
                {
                    if (hr == E_INVALIDARG)
                    {
                        IFC_RETURN(GetValueFromCDouble(
                            incomingValue,
                            rgbValue));
                    }
                    else
                    {
                        return hr;
                    }
                }

                m_value.SetColor(rgbValue);
            }
            break;

        case valueTypeHandle:
            {
                CType* incomingValueTyped = nullptr;
                IFC_RETURN(DoPointerCast(incomingValueTyped, incomingValue));
                m_value.SetTypeHandle(incomingValueTyped->GetReferencedTypeIndex());
            }
            break;

        case valueString:
            if (incomingValue)
            {
                CString* incomingValueTyped = do_pointer_cast<CString>(incomingValue);
                IFCCHECK_RETURN(incomingValueTyped != nullptr);
                m_value.SetString(incomingValueTyped->m_strString);
            }
            else
            {
                m_value.SetString(xstring_ptr::NullString());
            }
            break;

        case valuePoint:
            {
                CPoint* incomingValueTyped = nullptr;
                IFC_RETURN(DoPointerCast(incomingValueTyped, incomingValue));
                m_value.WrapPoint(&incomingValueTyped->m_pt);
            }
            break;

        case valueTextRange:
            {
                CTextRange* incomingValueTyped = nullptr;
                IFC_RETURN(DoPointerCast(incomingValueTyped, incomingValue));
                m_value.Set<valueTextRange>(incomingValueTyped->GetRange());
            }
            break;

        case valueSize:
            {
                CSize* incomingValueTyped = nullptr;
                IFC_RETURN(DoPointerCast(incomingValueTyped, incomingValue));
                m_value.WrapSize(&incomingValueTyped->m_size);
            }
            break;

        case valueThickness:
            {
                const XTHICKNESS* thickness = nullptr;

                IFC_RETURN(GetValueHelper(
                    incomingValue,
                    &CThickness::m_thickness,
                    KnownTypeIndex::Thickness,
                    thickness));

                m_value.WrapThickness(thickness);
            }
            break;

        case valueRect:
            {
                const XRECTF* rect = nullptr;

                IFC_RETURN(GetValueHelper(
                    incomingValue,
                    &CRect::m_rc,
                    KnownTypeIndex::Rect,
                    rect));

                m_value.WrapRect(rect);
            }
            break;

        case valueCornerRadius:
            {
                const XCORNERRADIUS* cornerRadius = nullptr;

                IFC_RETURN(GetValueHelper(
                    incomingValue,
                    &CCornerRadius::m_cornerRadius,
                    KnownTypeIndex::CornerRadius,
                    cornerRadius));

                m_value.WrapCornerRadius(cornerRadius);
            }
            break;

        case valueGridLength:
            {
                CGridLength* incomingValueTyped = nullptr;
                IFC_RETURN(DoPointerCast(incomingValueTyped, incomingValue));
                m_value.WrapGridLength(&incomingValueTyped->m_gridLength);
            }
            break;

        case valueVO:
            switch (pDP->GetPropertyType()->GetIndex())
            {
                case KnownTypeIndex::Duration:
                    m_vo = do_pointer_cast<CDuration>(incomingValue)->ValueWrapper();
                    break;

                case KnownTypeIndex::RepeatBehavior:
                    m_vo = do_pointer_cast<CRepeatBehavior>(incomingValue)->ValueWrapper();
                    break;

                case KnownTypeIndex::KeyTime:
                    m_vo = do_pointer_cast<CKeyTime>(incomingValue)->ValueWrapper();
                    break;

                default:
                    if (failOnUnhandled)
                    {
                        ASSERT(false);
                        IFC_RETURN(E_INVALIDARG);
                    }
                    break;
            }

            m_value.Wrap<valueVO>(m_vo);

            break;

        default:
            if (failOnUnhandled)
            {
                ASSERT(false);
                IFC_RETURN(E_INVALIDARG);
            }
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Float(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueFloat);

    float incomingValue = pValue->AsFloat();

    switch (storageType)
    {
        case valueObject:
        {
            // Box double.
            CREATEPARAMETERS cp(m_core);
            IFC_RETURN(CDouble::Create(m_obj.ReleaseAndGetAddressOf(), &cp));
            static_cast<CDouble*>(m_obj.get())->m_eValue = incomingValue;
            m_value.WrapObjectNoRef(m_obj.get());
            break;
        }

        case valueDouble:
            m_value.SetDouble(incomingValue);
            break;

        case valueBool:
            m_value.SetBool(incomingValue != 0.0f);
            break;

        case valueEnum:
            m_value.SetEnum(static_cast<uint32_t>(incomingValue));
            break;

        case valueEnum8:
            ASSERT(incomingValue <= static_cast<decltype(incomingValue)>(UINT8_MAX));
            m_value.SetEnum8(static_cast<uint8_t>(incomingValue));
            break;

        case valueSigned:
            m_value.SetSigned(static_cast<XINT32>(incomingValue));
            break;

        case valueThickness:
            m_primitive._thickness.left = incomingValue;
            m_primitive._thickness.right = incomingValue;
            m_primitive._thickness.top = incomingValue;
            m_primitive._thickness.bottom = incomingValue;
            m_value.WrapThickness(&m_primitive._thickness);
            break;

        case valueRect:
            m_primitive._rect.Height = incomingValue;
            m_primitive._rect.Width = incomingValue;
            m_primitive._rect.X = incomingValue;
            m_primitive._rect.Y = incomingValue;
            m_value.WrapRect(&m_primitive._rect);
            break;

        case valueCornerRadius:
            m_primitive._cornerRadius.bottomLeft = incomingValue;
            m_primitive._cornerRadius.bottomRight = incomingValue;
            m_primitive._cornerRadius.topLeft = incomingValue;
            m_primitive._cornerRadius.topRight = incomingValue;
            m_value.WrapCornerRadius(&m_primitive._cornerRadius);
            break;

        case valueGridLength:
            m_primitive._gridLength.type = DoubleUtil::IsNaN(incomingValue) ? GridUnitType::Auto : GridUnitType::Pixel;
            m_primitive._gridLength.value = incomingValue;
            m_value.WrapGridLength(&m_primitive._gridLength);
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
        }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Int(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType valueType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueSigned);

    XINT32 incomingValue = pValue->AsSigned();

    switch (valueType)
    {
        case valueBool:
            m_value.SetBool(incomingValue != 0);
            break;

        case valueEnum:
            m_value.SetEnum(static_cast<uint32_t>(incomingValue));
            break;

        case valueEnum8:
            ASSERT(incomingValue <= static_cast<decltype(incomingValue)>(UINT8_MAX));
            m_value.SetEnum8(static_cast<uint8_t>(incomingValue));
            break;

        case valueColor:
            m_value.SetColor(static_cast<XUINT32>(incomingValue));
            break;

        case valueFloat:
            m_value.SetFloat(static_cast<FLOAT>(incomingValue));
            break;

        case valueDouble:
            m_value.SetDouble(static_cast<DOUBLE>(incomingValue));
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Enum(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->IsEnum());

    uint32_t incomingValue = 0;
    KnownTypeIndex incomingType = KnownTypeIndex::UnknownType;

    // can safely ignore hresult since we know the type of pValue is valueEnum*
    IGNOREHR(pValue->GetEnum(incomingValue, incomingType));

    switch (storageType)
    {
        case valueEnum:
            m_value.SetEnum(incomingValue, incomingType);
            break;

        case valueEnum8:
            ASSERT(incomingValue <= static_cast<decltype(incomingValue)>(UINT8_MAX));
            m_value.SetEnum8(static_cast<uint8_t>(incomingValue), incomingType);
            break;

        case valueSigned:
            m_value.SetSigned(incomingValue);
            break;

        case valueBool:
            m_value.SetBool(incomingValue != 0);
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Bool(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueBool);

    bool incomingValue = pValue->AsBool();

    switch (storageType)
    {
        case valueEnum:
            m_value.SetEnum(incomingValue);
            break;

        case valueEnum8:
            ASSERT(incomingValue <= static_cast<decltype(incomingValue)>(UINT8_MAX));
            m_value.SetEnum8(static_cast<uint8_t>(incomingValue));
            break;

        case valueSigned:
            m_value.SetSigned(incomingValue);
            break;

        case valueBool:
            m_value.SetBool(!!incomingValue);
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Size(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueSize);

    if (storageType == valueObject)
    {
        CREATEPARAMETERS cp(m_core, *pValue);
        IFC_RETURN(CSize::Create(m_obj.ReleaseAndGetAddressOf(), &cp));
        m_value.WrapObjectNoRef(m_obj.get());
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Point(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valuePoint);

    if (storageType == valueObject &&
        pDP->GetPropertyType()->m_nIndex == KnownTypeIndex::Point)
    {
        CREATEPARAMETERS cp(m_core);
        IFC_RETURN(CPoint::Create(m_obj.ReleaseAndGetAddressOf(), &cp));
        static_cast<CPoint*>(m_obj.get())->m_pt = *pValue->AsPoint();
        m_value.WrapObjectNoRef(m_obj.get());
        pValue = &m_value;
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_TextRange(
    _In_ const CDependencyProperty* dp,
    _In_ ValueType storageType,
    _Inout_ CValue*& valueInOut)
{
    ASSERT(valueInOut->GetType() == valueTextRange);

    if (storageType == valueObject &&
        dp->GetPropertyType()->m_nIndex == KnownTypeIndex::TextRange)
    {
        CREATEPARAMETERS cp(m_core);
        IFC_RETURN(CTextRange::Create(m_obj.ReleaseAndGetAddressOf(), &cp));
        static_cast<CTextRange*>(m_obj.get())->SetRange(valueInOut->As<valueTextRange>());
        m_value.WrapObjectNoRef(m_obj.get());
        valueInOut = &m_value;
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Null(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueNull);

    switch (storageType)
    {
        case valuePoint:
            m_value.WrapPoint(nullptr);
            break;

        case valueTextRange:
            m_value.Set<valueTextRange>({ 0, 0 });
            break;

        case valueSize:
            m_value.WrapSize(nullptr);
            break;

        case valueRect:
            if (pDP->IsNullable())
            {
                m_value.SetNull();
            }
            else
            {
                m_value.WrapRect(nullptr);
            }
            break;

        case valueCornerRadius:
            m_value.WrapCornerRadius(nullptr);
            break;

        case valueThickness:
            m_value.WrapThickness(nullptr);
            break;

        case valueFloatArray:
            m_value.WrapFloatArray(0, nullptr);
            break;

        case valuePointArray:
            m_value.WrapPointArray(0, nullptr);
            break;

        case valueObject:
            m_value.WrapObjectNoRef(nullptr);
            break;

        case valueVO:
            m_value.Wrap<valueVO>(nullptr);
            break;

        case valueAny:
            m_value.SetNull();
            break;

        case valueString:
            m_value.SetString(xstring_ptr::NullString());
            break;

        case valueTypeHandle:
            m_value.SetTypeHandle(KnownTypeIndex::UnknownType);
            break;

        default:
            if (pDP->IsNullable())
            {
                m_value.SetNull();
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_DynamicToObject(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    if (storageType == valueObject)
    {
        CREATEPARAMETERS cp(m_core, *pValue);
        const CREATEPFN pfnCreate = pDP->GetPropertyType()->GetCoreConstructor();
        IFC_RETURN(pfnCreate(m_obj.ReleaseAndGetAddressOf(), &cp));
        m_value.WrapObjectNoRef(m_obj.get());
        pValue = &m_value;
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::ValueObjectFromDouble(
    KnownTypeIndex type,
    double value)
{
    switch (type)
    {
        case KnownTypeIndex::Duration:
            m_vo = DurationVOHelper::CreateTimeSpan(
                m_core,
                value);
            break;

        case KnownTypeIndex::RepeatBehavior:
            m_vo = RepeatBehaviorVOHelper::CreateDuration(
                m_core,
                value);
            break;

        case KnownTypeIndex::KeyTime:
            m_vo = KeyTimeVOHelper::CreateTimeSpan(
                m_core,
                value);
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
    }

    m_value.Wrap<valueVO>(m_vo);

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::ValueObjectFromString(
    KnownTypeIndex type,
    _In_ const xstring_ptr_view& value)
{
    switch (type)
    {
        case KnownTypeIndex::Duration:
            {
                DirectUI::DurationType durationType = DurationVO::s_defaultDurationType;
                double timeSpan = DurationVO::s_defaultTimeSpanInSec;

                IFC_RETURN(DurationFromString(
                    value,
                    &durationType,
                    &timeSpan));

                m_vo = DurationVOHelper::Create(
                    m_core,
                    durationType,
                    timeSpan);
            }
            break;

        case KnownTypeIndex::RepeatBehavior:
            {
                DirectUI::RepeatBehaviorType repeatBehaviorType = RepeatBehaviorVO::s_defaultRepeatBehaviorType;
                double duration = RepeatBehaviorVO::s_defaultDurationInSec;
                float count = RepeatBehaviorVO::s_defaultCount;

                IFC_RETURN(RepeatBehaviorFromString(
                    value,
                    &repeatBehaviorType,
                    &duration,
                    &count));

                m_vo = RepeatBehaviorVOHelper::Create(
                    m_core,
                    repeatBehaviorType,
                    duration,
                    count);
            }
            break;

        case KnownTypeIndex::KeyTime:
            {
                double timeSpan = KeyTimeVO::s_defaultTimeSpanInSec;

                IFC_RETURN(KeyTimeFromString(
                    value,
                    &timeSpan));

                m_vo = KeyTimeVOHelper::CreateTimeSpan(
                    m_core,
                    timeSpan);
            }
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
    }

    m_value.Wrap<valueVO>(m_vo);

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Double(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueDouble);

    if (storageType == valueObject &&
        pDP->GetPropertyTypeIndex() == KnownTypeIndex::TimeSpan)
    {
        CREATEPARAMETERS cp(m_core, *pValue);

        const CREATEPFN pfnCreate = pDP->GetPropertyType()->GetCoreConstructor();
        IFC_RETURN(pfnCreate(m_obj.ReleaseAndGetAddressOf(), &cp));

        m_value.WrapObjectNoRef(m_obj.get());
        pValue = &m_value;
    }
    else if (storageType == valueVO)
    {
        IFC_RETURN(ValueObjectFromDouble(
            pDP->GetPropertyTypeIndex(),
            pValue->As<valueDouble>()));
    }
    else if (storageType == valueFloat)
    {
        m_value.SetFloat(static_cast<XFLOAT>(pValue->AsDouble()));
    }
    else if (storageType == valueSigned)
    {
        m_value.SetSigned(static_cast<INT32>(pValue->AsDouble()));
    }
    // This only exists for Windows 8.1 compat, where a value could come in as a valueFloat, and be assigned to an enum property (FontWeight). In
    // Threshold, such a value might now come in as valueDouble, so we need to support a conversion from double to enum.
    else if (storageType == valueEnum)
    {
        m_value.SetEnum(static_cast<UINT32>(pValue->AsDouble()));
    }
    else if (storageType == valueEnum8)
    {
        ASSERT(pValue->AsDouble() <= static_cast<decltype(pValue->AsDouble())>(UINT8_MAX));
        m_value.SetEnum8(static_cast<uint8_t>(pValue->AsDouble()));
    }
    else if (storageType == valueGridLength)
    {
        m_primitive._gridLength.type = DoubleUtil::IsNaN(pValue->AsDouble()) ? GridUnitType::Auto : GridUnitType::Pixel;
        m_primitive._gridLength.value = static_cast<float>(pValue->AsDouble());
        m_value.WrapGridLength(&m_primitive._gridLength);
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_TimeSpan(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueTimeSpan);

    if (storageType == valueObject &&
        pDP->GetPropertyTypeIndex() == KnownTypeIndex::TimeSpan)
    {
        CREATEPARAMETERS cp(m_core, *pValue);

        const CREATEPFN pfnCreate = pDP->GetPropertyType()->GetCoreConstructor();
        IFC_RETURN(pfnCreate(m_obj.ReleaseAndGetAddressOf(), &cp));

        m_value.WrapObjectNoRef(m_obj.get());
        pValue = &m_value;
    }
    else if (storageType == valueVO)
    {
        using namespace std::chrono;

        IFC_RETURN(ValueObjectFromDouble(
            pDP->GetPropertyTypeIndex(),
            TimeSpanUtil::ToSeconds(pValue->As<valueTimeSpan>())));
    }
    else if (storageType == valueFloat)
    {
        m_value.SetFloat(static_cast<XFLOAT>(pValue->AsTimeSpan().Duration));
    }
    else if (storageType == valueSigned)
    {
        m_value.SetSigned(static_cast<INT32>(pValue->AsTimeSpan().Duration));
    }
    // This only exists for Windows 8.1 compat, where a value could come in as a valueFloat, and be assigned to an enum property (FontWeight). In
    // Threshold, such a value might now come in as valueDouble, so we need to support a conversion from double to enum.
    else if (storageType == valueEnum)
    {
        m_value.SetEnum(static_cast<UINT32>(pValue->AsTimeSpan().Duration));
    }
    else if (storageType == valueEnum8)
    {
        ASSERT(pValue->AsTimeSpan().Duration <= static_cast<decltype(pValue->AsTimeSpan().Duration)>(UINT8_MAX));
        m_value.SetEnum8(static_cast<uint8_t>(pValue->AsTimeSpan().Duration));
    }
    else if (storageType == valueGridLength)
    {
        // Keeping this to retain original behavior.
        m_primitive._gridLength.type = DoubleUtil::IsNaN(static_cast<XFLOAT>(pValue->AsTimeSpan().Duration)) ? GridUnitType::Auto : GridUnitType::Pixel;
        m_primitive._gridLength.value = static_cast<float>(pValue->AsTimeSpan().Duration);
        m_value.WrapGridLength(&m_primitive._gridLength);
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_Thickness(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueThickness);

    // This conversion is necessary for app compat. In Windows 8.1, we allowed
    // ThemeResource references to thickness types on float properties, and some
    // apps have taken a dependency on that support.

    if (storageType == valueFloat)
    {
        m_value.SetFloat(pValue->AsThickness()->left);
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT ValueBuffer::RepackageValue_String(
    _In_ const CDependencyProperty* pDP,
    _In_ ValueType storageType,
    _Inout_ CValue*& pValue)
{
    ASSERT(pValue->GetType() == valueString);

    CREATEPARAMETERS cp(m_core, *pValue);

    // If the property is a length (i.e. FrameworkElement_Width). We should go through the CLengthConverter since that knows how to convert "Auto".
    // We check for this first since we don't create a CDependencyObject, just the CValue itself.
    if (pDP->IsGridLengthProperty())
    {
        IFC_RETURN(CLengthConverter::CreateCValue(&cp, m_value));
        pValue = &m_value;

        // CLengthConverter creates float CValues (for historical reasons; FrameworkElement.Height/Width is a "grid length property"
        // that, despite being of type 'double' is stored as a float). This can lead to incorrect behavior if we have a "grid length property"
        // that is actually stored as a double, e.g. SplitView_OpenPaneLength. Therefore, if the property shouldn't be stored as a float,
        // we need to make sure it gets repackaged as a double.
        if (storageType == valueDouble && !pDP->StoreDoubleAsFloat())
        {
            // CLengthConverter is expected to return a float CValue
            ASSERT(pValue->GetType() == valueFloat);
            IFC_RETURN(RepackageValue_Float(pDP, storageType, pValue));
        }
    }
    else
    {
        // If the value coming in is a string, but the property is *exactly* of type
        // INDEX_DEPENDENCYOBJECT, then we create a CString, otherwise
        // call the type converter on the incoming string
        if (pDP->GetPropertyType()->m_nIndex == KnownTypeIndex::DependencyObject || pDP->GetPropertyType()->m_nIndex == KnownTypeIndex::Object)
        {
            IFC_RETURN(CString::Create(m_obj.ReleaseAndGetAddressOf(), &cp));
        }
        else if (storageType == valueVO)
        {
            // Don't repackage to object, since VOs are stored as VOs.
            IFC_RETURN(ValueObjectFromString(
                pDP->GetPropertyTypeIndex(),
                pValue->AsString()));
            return S_OK;
        }
        else
        {
            const CREATEPFN pfnCreate = pDP->GetPropertyType()->GetCoreConstructor();
            if (!pfnCreate)
            {
                IFC_RETURN(E_FAIL);
            }

            IFC_RETURN(pfnCreate(m_obj.ReleaseAndGetAddressOf(), &cp));
        }

        m_value.WrapObjectNoRef(m_obj.get());
        pValue = &m_value;

        IFC_RETURN(RepackageValue_Object(pDP, storageType, pValue, false));
    }

    return S_OK;
}