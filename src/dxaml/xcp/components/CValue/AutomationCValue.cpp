// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <AutomationCValue.h>
#include <TypeTableStructs.h>
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <ThemeResourceExtension.h>
#include "ThemeResource.h"
#include <CValue.h>
#include <CValueConvert.h>
#include <Primitives.h>

namespace Automation
{
    CValue::~CValue()
    {
        ReleaseAndReset();
    }

    // Release memory/reference for most of the supported CValue types.
    void CValue::ReleaseAndReset()
    {
        if (m_fOwnsValue)
        {
            // Avoid re-entrancy when calling SetNull().
            m_fOwnsValue = FALSE;

            switch (m_type)
            {
                case valueAny:
                case valueFloat:
                case valueSigned:
                case valueInt64:
                case valueTypeHandle:
                case valueBool:
                case valueEnum:
                case valueEnum8:
                case valueColor:
                case valueNull:
                case valueDouble:
                    // Wholly contained in CValue union, no memory to free.
                    break;

                case valueString:
                    m_estrValue.Reset();
                    ASSERT(m_internalCount == 0);

                    // Zero-length null string is a valid string construct, so leaving it in this state.

                    break;

                case valuePoint:
                case valueRect:
                case valueSize:
                case valueGridLength:
                case valueThickness:
                case valueCornerRadius:
                    // Types which can be treated as XFLOAT* for purpose of cleanup.

                    delete m_peValue;

                    // Unlike valueString, not all of the types are valid without data.
                    //  (What's the point of a valuePoint with no point?)
                    // So set to null.
                    SetNull();

                    break;

                case valueFloatArray:
                    // Types which can be treated as an array of XFLOAT for purpose of cleanup.

                    delete[] m_peValue;

                    // Unlike valueString, not all of the types are valid without data.
                    //  (What's the point of a valuePoint with no point?)
                    // So set to null.
                    SetNull();

                    break;
                case valueObject:
                    ReleaseInterface(m_pdoValue);
                    break;
                case valueThemeResource:
                    ReleaseInterface(m_pThemeResource);
                    break;
                case valueIInspectable:
                    ReleaseInterface(m_pInspValue);
                    break;
                case valueIUnknown:
                    ReleaseInterface(m_pUnkValue);
                    break;
                case valueSignedArray:
                    delete[] m_piValue;
                    SetNull();
                    break;
                case valueDoubleArray:
                    delete[] m_pdfValue;
                    SetNull();
                    break;
            }
        }
    }

    void CValue::WrapValue(_In_ const CValue& value)
    {
        ReleaseAndReset();

        CountTypeAndFlags = value.EncodeCountTypeAndFlags();
        switch (m_type)
        {
            case valueString:
                // Make sure to correctly add-ref the runtime strings
                m_estrValue = value.m_estrValue.Clone();
                break;
            default:
                m_dValue = value.m_dValue;
                m_fOwnsValue = FALSE;
                break;
        }
    }

    void CValue::SetObjectAddRef(_In_opt_ CDependencyObject *val)
    {
        ResetValue(valueObject, 0);
        SetInterface(m_pdoValue, val);
    }

    void CValue::SetThemeResourceAddRef(_In_ CThemeResource *val)
    {
        ResetValue(valueThemeResource, 0);
        SetInterface(m_pThemeResource, val);
    }

    ctl::ComPtr<IInspectable> CValue::DetachIInspectable()
    {
        ASSERT(m_type == valueNull || m_type == valueIInspectable);
        ASSERT(m_fOwnsValue);

        ctl::ComPtr<IInspectable> obj;
        obj.Attach(m_pInspValue);
        m_fOwnsValue = FALSE;
        SetNull();

        return obj;
    }

    ctl::ComPtr<IUnknown> CValue::DetachIUnknown()
    {
        ASSERT(m_type == valueNull || m_type == valueIUnknown);
        ASSERT(m_fOwnsValue);

        ctl::ComPtr<IUnknown> obj;
        obj.Attach(m_pUnkValue);
        m_fOwnsValue = FALSE;
        SetNull();

        return obj;
    }

    xref_ptr<CDependencyObject> CValue::DetachObject()
    {
        ASSERT(m_type == valueNull || m_type == valueObject);
        ASSERT(m_fOwnsValue);

        xref_ptr<CDependencyObject> obj;
        obj.attach(m_pdoValue);
        m_fOwnsValue = FALSE;
        SetNull();

        return obj;
    }

    KnownTypeIndex CValue::GetTypeIndex() const
    {
        if (m_type == valueObject && m_pdoValue)
        {
            if (m_pdoValue->GetClassInformation()->IsEnum())
            {
                return static_cast<CEnumerated *>(m_pdoValue)->GetEnumTypeIndex();
            }
            return m_pdoValue->GetTypeIndex();
        }
        else if (m_type == valueEnum || m_type == valueEnum8)
        {
            return m_enumValue.eTypeIndex;
        }
        return KnownTypeIndex::UnknownType;
    }

    //-------------------------------------------------------------------------------
    //
    //  Type-specific value comparison.
    //
    //-------------------------------------------------------------------------------
    bool
        CValue::operator==(
            const CValue &other
            ) const
    {
        if (m_type == valueIInspectable || other.m_type == valueIInspectable)
        {
            CValue leftRawValue;
            CValue rightRawValue;

            VERIFYHR(CValueConvert::EnsurePropertyValueUnboxed(
                *this,
                leftRawValue));

            VERIFYHR(CValueConvert::EnsurePropertyValueUnboxed(
                other,
                rightRawValue));

            if (leftRawValue.m_type == valueIInspectable && rightRawValue.m_type == valueIInspectable)
            {
                return (leftRawValue.m_pInspValue == rightRawValue.m_pInspValue);
            }
            // If we're still dealing with one IInspectable, then we know the values can't be equal.
            else if (leftRawValue.m_type == valueIInspectable || rightRawValue.m_type == valueIInspectable)
            {
                return false;
            }

            return (leftRawValue == rightRawValue);
        }

        if (m_type == other.m_type)
        {
            switch (m_type)
            {
                case valueFloat:
                    return m_eValue == other.m_eValue;

                case valueSigned:
                    return m_iValue == other.m_iValue;

                case valueInt64:
                    return m_lValue == other.m_lValue;

                case valueDateTime:
                case valueTimeSpan:
                    {
                        return m_lValue == other.m_lValue;
                    }

                case valueTypeHandle:
                    return m_tValue == other.m_tValue;

                case valueBool:
                    return (!!m_nValue) == (!!other.m_nValue);

                case valueEnum:
                case valueEnum8:
                    return m_enumValue.nValue == other.m_enumValue.nValue;

                case valueColor:
                    return m_nValue == other.m_nValue;

                case valueObject:
                case valueIUnknown:
                case valuePointer:
                    return m_pvValue == other.m_pvValue;

                case valueString:
                    return !!AsString().Equals(other.AsString());

                case valueFloatArray:
                case valuePointArray:
                    return (GetArrayElementCount() == other.GetArrayElementCount()) && (m_peValue == other.m_peValue);

                case valueSignedArray:
                    return (GetArrayElementCount() == other.GetArrayElementCount()) && (m_piValue == other.m_piValue);
                case valueDoubleArray:
                    return (GetArrayElementCount() == other.GetArrayElementCount()) && (m_pdfValue == other.m_pdfValue);

                case valueSize:
                case valuePoint:
                    return (m_peValue[0] == other.m_peValue[0]) && (m_peValue[1] == other.m_peValue[1]);

                case valueDouble:
                    return (m_dValue == other.m_dValue);

                case valueCornerRadius:
                case valueThickness:
                case valueRect:
                    return (m_peValue[0] == other.m_peValue[0]) && (m_peValue[1] == other.m_peValue[1])
                        && (m_peValue[2] == other.m_peValue[2]) && (m_peValue[3] == other.m_peValue[3]);

                case valueInternalHandler:
                    return m_pHandler == other.m_pHandler;

                case valueGridLength:
                    return(*(XUINT32*)m_peByteValue == *(XUINT32*)other.m_peByteValue
                        && *(XFLOAT*)(m_peByteValue + sizeof(XUINT32)) == *(XFLOAT*)(other.m_peByteValue + sizeof(XUINT32)));

                case valueNull:
                    return true;
                case valueAny:
                case valueThemeResource:
                    return false;
                default:
                    ASSERT(false);
                    return false;
            }
        }
        // These checks are added to prevent additional OnPropertyChanged notifications from
        // being fired for the case where we assign a binding to a property. The binding expression
        // unwraps into a valueDouble CValue while the default CValue type of the GetDefaultValue
        // codepath creates valueFloat CValues for Double properties.
        //
        // A follow up bug, MSFT: 810951, has been opened to fix GetDefaultValue. The fix was considered
        // too high-risk for this M1.3 bug.
        else if (m_type == valueDouble && other.m_type == valueFloat)
        {
            return m_dValue == other.m_eValue;
        }
        else if (m_type == valueFloat && other.m_type == valueDouble)
        {
            return m_eValue == other.m_dValue;
        }


        return false;
    }

    // copy assignment operator
    Automation::CValue& CValue::operator=(const Automation::CValue &other)
    {
        VERIFYHR(CopyConverted(other));
        return (*this);
    }

    _Check_return_ HRESULT CValue::SetString(_In_ HSTRING val)
    {
        xstring_ptr xstr;
        HRESULT hr = xstring_ptr::CloneRuntimeStringHandle(val, &xstr);
        if (SUCCEEDED(hr))
        {
            SetString(std::move(xstr));
        }
        return hr;
    }

    //-------------------------------------------------------------------------------
    //
    //  CValue::CopyConverted
    //
    //  Copy contents of the given CValue into this one.
    //
    //-------------------------------------------------------------------------------
    _Check_return_ HRESULT
        CValue::CopyConverted(_In_ const CValue& source)
    {
        if (this != &source)
        {
            ReleaseAndReset();
            IFC_RETURN(ConvertForManaged(this, source));
        }

        return S_OK;
    }

    // Convert the type for Managed consumption.
    template<typename TSource, typename TDest>
    _Check_return_ HRESULT CValue::ConvertForManaged(
        _Out_ TDest* pData,
        _In_ const TSource& value)
    {
        // The return type from this call can be boxed or unboxed so account for both
        switch (value.GetType())
        {
            case valueAny: // Nothing to do here. Move along.
            case valueNull:
                pData->SetNull();
                break;
            case valueFloat:
                pData->SetFloat(value.AsFloat());
                break;

            case valueSigned:
                pData->SetSigned(value.AsSigned());
                break;

            case valueTypeHandle:
                pData->SetTypeHandle(value.AsTypeHandle());
                break;

            case valueInt64:
                pData->SetInt64(value.AsInt64());
                break;

            case valueColor:
                pData->SetColor(value.AsColor());
                break;

            case valueTimeSpan:
                {
                    wf::TimeSpan timeSpan = {};
                    IFC_RETURN(value.GetTimeSpan(timeSpan));
                    pData->SetTimeSpan(timeSpan);
                }
                break;

            case valueDateTime:
                {
                    wf::DateTime date = {};
                    IFC_RETURN(value.GetDateTime(date));
                    pData->SetDateTime(date);
                }
                break;

            case valueEnum:
            case valueEnum8:
                {
                    uint32_t enumValue = 0;
                    KnownTypeIndex typeIndex = KnownTypeIndex::UnknownType;
                    VERIFYHR(value.GetEnum(enumValue, typeIndex));
                    pData->SetEnum(enumValue, typeIndex);
                }
                break;

            case valueString:
                pData->SetString(std::move(value.AsString()));
                break;

            case valueBool:
                pData->SetBool(value.AsBool());
                break;

            case valuePoint:
                {
                    XPOINTF *pXf = new XPOINTF;
                    *pXf = *value.AsPoint();
                    pData->SetPoint(pXf);
                }
                break;

            case valueThemeResource:
                {
                    pData->SetThemeResourceAddRef(value.AsThemeResource());
                }
                break;

            case valueDouble:
                pData->SetDouble(value.AsDouble());
                break;

            case valueSize:
                {
                    XSIZEF *pXf = new XSIZEF;
                    *pXf = *value.AsSize();
                    pData->SetSize(pXf);
                }
                break;

            case valueThickness:
                {
                    XTHICKNESS* pXf = new XTHICKNESS;
                    *pXf = *value.AsThickness();
                    pData->SetThickness(pXf);
                }
                break;

            case valueCornerRadius:
                {
                    XCORNERRADIUS* pXf = new XCORNERRADIUS;
                    *pXf = *value.AsCornerRadius();
                    pData->SetCornerRadius(pXf);
                }
                break;

            case valueGridLength:
                {
                    XGRIDLENGTH* pXf = new XGRIDLENGTH;
                    *pXf = *value.AsGridLength();
                    pData->SetGridLength(pXf);
                }
                break;

            case valueRect:
                {
                    XRECTF* pXf = new XRECTF;
                    *pXf = *value.AsRect();
                    pData->SetRect(pXf);
                }
                break;

            case valueFloatArray:
                {
                    XUINT32 count = value.GetArrayElementCount();
                    XFLOAT* pArray = new XFLOAT[count];
                    memcpy(pArray, value.AsFloatArray(), sizeof(XFLOAT) * count);
                    pData->SetFloatArray(count, pArray);
                }
                break;

            case valuePointArray:
                {
                    XUINT32 count = value.GetArrayElementCount();
                    XPOINTF* pArray = new XPOINTF[count];
                    memcpy(pArray, value.AsPointArray(), sizeof(XPOINTF) * count);
                    pData->SetPointArray(count, pArray);
                }
                break;

            case valueSignedArray:
                {
                    XINT32* pArray = new XINT32[value.GetArrayElementCount()];
                    memcpy(pArray, value.AsSignedArray(), sizeof(XINT32) * value.GetArrayElementCount());
                    pData->SetSignedArray(value.GetArrayElementCount(), pArray);
                }
                break;

            case valueDoubleArray:
                {
                    XDOUBLE* pArray = new XDOUBLE[value.GetArrayElementCount()];
                    memcpy(pArray, value.AsDoubleArray(), sizeof(XDOUBLE) * value.GetArrayElementCount());
                    pData->SetDoubleArray(value.GetArrayElementCount(), pArray);
                }
                break;

            case valueObject://Boxed Type
                {
                    TSource temp;

                    IFC_RETURN(CValueConvert::EnsureCDependencyObjectValueUnboxed<>(
                        value,
                        temp));

                    pData->SetObjectAddRef(temp.AsObject());
                }
                break;

            case valueIUnknown:
                pData->SetIUnknownAddRef(value.AsIUnknown());
                break;

            case valueIInspectable:
                pData->SetIInspectableAddRef(value.AsIInspectable());
                break;

            case valuePointer:
                pData->SetPointer(value.AsPointer());
                break;

                // This is a static method pointer to internal (native) code.
                // There is no current need to convert this for consumption by
                //  the managed code layer.  If the need should come up, insert
                //  code here to handle conversion.
                // Until then, fall through to the default case of E_FAIL.
            case valueInternalHandler:
            default:
                IFC_RETURN(E_FAIL);
                break;
        }

        return S_OK;
    }

    _Check_return_ HRESULT CValue::ConvertFrom(
        _In_ const ::CValue& source)
    {
        ReleaseAndReset();
        IFC_RETURN(ConvertForManaged(this, source));
        return S_OK;
    }

    _Check_return_ HRESULT CValue::ConvertTo(
        _Out_::CValue& dest) const
    {
        IFC_RETURN(ConvertForManaged(&dest, *this));
        return S_OK;
    }

}
